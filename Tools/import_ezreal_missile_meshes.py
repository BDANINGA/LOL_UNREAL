"""Import original OBJ meshes and create procedural materials without touching old assets."""
import unreal as u
from pathlib import Path
assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
assert not u.EditorLoadingAndSavingUtils.get_dirty_content_packages()
folder='/Game/Level/ezreal/FX/Missiles'
lib=u.MaterialEditingLibrary
assets=u.AssetToolsHelpers.get_asset_tools()
def material(name,attack,energy,style=None):
    path=folder+'/'+name
    mat=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else assets.create_asset(name,folder,u.Material,u.MaterialFactoryNew())
    mat.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE if energy else u.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided',True)
    def node(cls,index,x,y):
        return u.find_object(mat,cls.__name__+'_'+str(index)) or lib.create_material_expression(mat,cls,x,y)
    uv=node(u.MaterialExpressionTextureCoordinate,0,-500,0)
    time=node(u.MaterialExpressionTime,0,-500,150)
    shader=node(u.MaterialExpressionCustom,0,-180,0)
    shader.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs=[]
    for key in ['UV','T']:
        inp=u.CustomInput();inp.set_editor_property('input_name',key);inputs.append(inp)
    shader.set_editor_property('inputs',inputs)
    if energy:
        code='''float edge=pow(saturate(1-abs(UV.y*2-1)),1.7);
float ends=smoothstep(0,0.22,UV.x)*(1-smoothstep(0.9,1,UV.x));
float flow=0.72+0.28*sin(UV.x*32-T*18);
float3 tint=lerp(float3(0.025,0.5,0.85),float3(1,0.63,0.16),smoothstep(0.35,0.85,UV.x));
return tint*edge*ends*flow*1.8;'''
        if attack: code=code.replace('float3(0.025,0.5,0.85)','float3(0.75,0.28,0.025)')
    else:
        code='''float head=smoothstep(0.15,0.9,UV.x);
float3 tint=lerp(float3(0.035,0.35,0.65),float3(1,0.86,0.48),head);
float facet=0.8+0.2*pow(abs(cos(UV.y*6.283)),3);
return tint*facet*(0.75+head*1.05);'''
        if attack: code=code.replace('float3(0.035,0.35,0.65)','float3(0.85,0.3,0.025)')
    if style=='W':
        code=('''float edge=pow(saturate(1-abs(UV.y*2-1)),1.5);
float pulse=0.78+0.22*sin(UV.x*37.699-T*5);
float3 tint=lerp(float3(0.025,0.5,0.85),float3(1,0.63,0.16),pow(saturate(sin(UV.x*18.85-T*3)),5));
return tint*edge*pulse*1.6;''' if energy else
'''float pulse=0.85+0.15*sin(UV.x*25.133-T*5);
float rim=pow(abs(cos(UV.y*6.283)),3);
return lerp(float3(1,0.63,0.16),float3(1,0.86,0.48),rim)*pulse*(0.95+0.5*rim);''')
    shader.set_editor_property('code',code)
    assert lib.connect_material_expressions(uv,'',shader,'UV')
    assert lib.connect_material_expressions(time,'',shader,'T')
    assert lib.connect_material_property(shader,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    if energy:
        opacity=node(u.MaterialExpressionConstant,0,-180,240);opacity.set_editor_property('r',1.0)
        assert lib.connect_material_property(opacity,'',u.MaterialProperty.MP_OPACITY)
    lib.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
    return mat

for name,variant in globals().get('MISSILE_IMPORTS',[('SM_EzrealMysticShot','Q'),('SM_EzrealBasicBolt','Basic')]):
    core=material('M_Ezreal'+variant+'Core',variant=='Basic',False,variant)
    energy=material('M_Ezreal'+variant+'Energy',variant=='Basic',True,variant)
    task=u.AssetImportTask()
    task.set_editor_property('filename',str(Path(u.Paths.project_dir()).resolve().parent/'Tools/Art/Ezreal'/(name+'.obj')))
    task.set_editor_property('destination_path',folder)
    task.set_editor_property('destination_name',name)
    task.set_editor_property('automated',True)
    task.set_editor_property('replace_existing',True)
    task.set_editor_property('save',True)
    options=u.FbxImportUI()
    options.set_editor_property('import_mesh',True)
    options.set_editor_property('import_materials',False)
    options.set_editor_property('import_textures',False)
    options.set_editor_property('import_as_skeletal',False)
    options.set_editor_property('mesh_type_to_import',u.FBXImportType.FBXIT_STATIC_MESH)
    options.static_mesh_import_data.set_editor_property('combine_meshes',True)
    options.static_mesh_import_data.set_editor_property('auto_generate_collision',False)
    options.static_mesh_import_data.set_editor_property('convert_scene',False)
    options.static_mesh_import_data.set_editor_property('convert_scene_unit',False)
    task.set_editor_property('options',options)
    task.set_editor_property('factory',u.FbxFactory())
    assets.import_asset_tasks([task])
    mesh=u.load_asset(folder+'/'+name)
    assert isinstance(mesh,u.StaticMesh), task.get_editor_property('imported_object_paths')
    slots=mesh.get_editor_property('static_materials')
    print('SLOTS',name,[(str(s.get_editor_property('material_slot_name'))) for s in slots])
    assert len(slots)==2
    for i,slot in enumerate(slots):
        mesh.set_material(i,core if str(slot.get_editor_property('material_slot_name'))=='Core' else energy)
        u.get_editor_subsystem(u.StaticMeshEditorSubsystem).enable_section_cast_shadow(mesh,False,0,i)
    assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
    print('IMPORTED',mesh.get_path_name(),mesh.get_bounds())
