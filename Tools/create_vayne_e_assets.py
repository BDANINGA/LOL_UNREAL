"""Build original Condemn bolt and its silver/violet materials in the editor."""
import math
from pathlib import Path
import unreal as u

folder = '/Game/VFX/Vayne'
lib = u.MaterialEditingLibrary
assets = u.AssetToolsHelpers.get_asset_tools()

def material(name, glow=False):
    mat = u.load_asset(folder+'/'+name) or assets.create_asset(name, folder, u.Material, u.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat)
    mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    mat.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE if glow else u.BlendMode.BLEND_OPAQUE)
    tint = lib.create_material_expression(mat, u.MaterialExpressionVectorParameter, -400, 0)
    tint.set_editor_property('parameter_name', 'Tint')
    tint.set_editor_property('default_value', u.LinearColor(.6, .22, 1., 1.) if glow else u.LinearColor(.68, .74, .92, 1.))
    fade = lib.create_material_expression(mat, u.MaterialExpressionScalarParameter, -400, 200)
    fade.set_editor_property('parameter_name', 'Fade')
    fade.set_editor_property('default_value', 1.)
    shader = lib.create_material_expression(mat, u.MaterialExpressionCustom, -120, 0)
    shader.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
    names = ['Tint', 'Fade', 'UV']
    inputs = []
    for name in names:
        entry = u.CustomInput(); entry.set_editor_property('input_name', name); inputs.append(entry)
    shader.set_editor_property('inputs', inputs)
    shader.set_editor_property('code',
        'float2 p=UV*2-1; float soft=pow(saturate(1-dot(p,p)),1.6); return Tint*soft*Fade*3.0;' if glow else
        'float facet=.65+.35*abs(cos(UV.y*6.283185)); return Tint*facet*1.35;')
    uv = lib.create_material_expression(mat, u.MaterialExpressionTextureCoordinate, -400, 400)
    for node, name in [(tint, 'Tint'), (fade, 'Fade'), (uv, 'UV')]:
        assert lib.connect_material_expressions(node, '', shader, name)
    assert lib.connect_material_property(shader, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    if glow:
        opacity = lib.create_material_expression(mat, u.MaterialExpressionConstant, -120, 220)
        opacity.set_editor_property('r', 1.)
        lib.connect_material_property(opacity, '', u.MaterialProperty.MP_OPACITY)
    lib.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat, False)
    return mat

metal = material('M_VayneCondemnSilver')
glow = material('M_VayneCondemnGlow', True)
verts, faces = [], []
def v(p):
    verts.append(p); return len(verts)
def tri(a,b,c): faces.append((a,b,c))
def diamond(points):
    # Four-sided double pyramid: a broad, faceted silver arrowhead.
    front, back, ring = v(points[0]), v(points[1]), [v(p) for p in points[2:]]
    for i in range(4):
        tri(front, ring[i], ring[(i+1)%4]); tri(back, ring[(i+1)%4], ring[i])
diamond([(65,0,0),(-8,0,0),(12,19,0),(12,0,8),(12,-19,0),(12,0,-8)])
# Shaft and two swept rear vanes. Total length 132 cm, width 38 cm.
diamond([(30,0,0),(-67,0,0),(-45,3,0),(-45,0,3),(-45,-3,0),(-45,0,-3)])
for s in [-1,1]:
    diamond([(-24,0,0),(-62,0,0),(-52,s*15,0),(-45,s*6,3),(-38,0,0),(-45,s*6,-3)])
out = Path(u.Paths.project_dir()).resolve().parent/'Tools/Art/Vayne'
out.mkdir(parents=True, exist_ok=True)
obj = out/'SM_VayneCondemnBolt.obj'
lines = ['# Original Condemn bolt; forward +X, centimeters', 'o SM_VayneCondemnBolt', 's off']
lines += ['v %.6f %.6f %.6f'%p for p in verts]
lines += ['vt %.6f %.6f'%((p[0]+67)/132, (i%4)/3) for i,p in enumerate(verts)]
lines += ['usemtl Silver']
lines += ['f '+' '.join(f'{i}/{i}' for i in f) for f in faces]
obj.write_text('\n'.join(lines)+'\n', encoding='utf8')
task = u.AssetImportTask()
for key,value in dict(filename=str(obj), destination_path=folder, destination_name='SM_VayneCondemnBolt', automated=True, replace_existing=True, save=True).items():
    task.set_editor_property(key,value)
options = u.FbxImportUI()
for key,value in dict(import_mesh=True, import_materials=False, import_textures=False, import_as_skeletal=False, mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH).items():
    options.set_editor_property(key,value)
for key,value in dict(combine_meshes=True, auto_generate_collision=False, convert_scene=False, convert_scene_unit=False).items():
    options.static_mesh_import_data.set_editor_property(key,value)
task.set_editor_property('options',options)
task.set_editor_property('factory',u.FbxFactory())
assets.import_asset_tasks([task])
mesh = u.load_asset(folder+'/SM_VayneCondemnBolt')
assert isinstance(mesh,u.StaticMesh)
mesh.set_material(0,metal)
assert u.EditorAssetLibrary.save_loaded_asset(mesh,False)
print('VAYNE_E_ASSETS_READY',mesh.get_bounds())
