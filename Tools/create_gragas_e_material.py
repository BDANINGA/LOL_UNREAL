"""Two tapered curved wisps, aligned to particle velocity by the E emitter."""
import unreal as u
lib=u.MaterialEditingLibrary
folder='/Game/VFX/Gragas';name='M_GragasDashWisp';path=folder+'/'+name
mat=(u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,u.Material,u.MaterialFactoryNew()))
mat.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided',True)
lib.set_material_usage(mat,u.MaterialUsage.MATUSAGE_PARTICLE_SPRITES)
counts={}
def node(cls,x,y):
    i=counts.get(cls.__name__,0);counts[cls.__name__]=i+1
    return u.find_object(mat,cls.__name__+'_'+str(i)) or lib.create_material_expression(mat,cls,x,y)
uv=node(u.MaterialExpressionTextureCoordinate,-700,180)
shape=node(u.MaterialExpressionCustom,-480,180)
shape.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
inp=u.CustomInput();inp.set_editor_property('input_name','UV')
shape.set_editor_property('inputs',[inp])
shape.set_editor_property('code','''
float2 p=UV*2-1;
float taper=pow(saturate(1-p.y*p.y),1.5);
float bend=.48*(1-p.y*p.y)-.12;
float width=.025+.12*taper;
float main=pow(saturate(1-abs(p.x-bend)/width),1.4);
float echo=pow(saturate(1-abs(p.x+bend+.22)/(width*.65)),1.5)*.45;
return saturate((main+echo)*taper);
''')
assert lib.connect_material_expressions(uv,'',shape,'UV')
color=node(u.MaterialExpressionParticleColor,-700,-100)
glow=node(u.MaterialExpressionMultiply,-220,-100);glow.set_editor_property('const_b',2.6)
assert lib.connect_material_expressions(color,'RGB',glow,'A')
alpha=node(u.MaterialExpressionMultiply,-220,180)
assert lib.connect_material_expressions(shape,'',alpha,'A')
assert lib.connect_material_expressions(color,'A',alpha,'B')
assert lib.connect_material_property(glow,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
assert lib.connect_material_property(alpha,'',u.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
print('GRAGAS_E_WISP_MATERIAL_READY')
