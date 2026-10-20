"""Texture-free UI material: a transparent center and soft crimson screen edges."""
import unreal as u

path = '/Game/UI/Effects'
name = 'M_DamageVignette'
lib = u.MaterialEditingLibrary
mat = (u.load_asset(path + '/' + name) if u.EditorAssetLibrary.does_asset_exist(path + '/' + name)
       else u.AssetToolsHelpers.get_asset_tools().create_asset(name, path, u.Material, u.MaterialFactoryNew()))
mat.set_editor_property('material_domain', u.MaterialDomain.MD_UI)
mat.set_editor_property('blend_mode', u.BlendMode.BLEND_TRANSLUCENT)

def node(cls, x, y):
    return u.find_object(mat, cls.__name__ + '_0') or lib.create_material_expression(mat, cls, x, y)

uv = node(u.MaterialExpressionTextureCoordinate, -600, 100)
mask = node(u.MaterialExpressionCustom, -300, 100)
mask.set_editor_property('description', 'Soft rounded crimson border; center stays fully transparent')
mask.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT1)
item = u.CustomInput()
item.set_editor_property('input_name', 'UV')
mask.set_editor_property('inputs', [item])
mask.set_editor_property('code', '''
float2 p = abs(UV * 2.0 - 1.0);
float2 edge = smoothstep(0.64, 1.03, p);
float border = 1.0 - (1.0 - edge.x) * (1.0 - edge.y);
return border * border * 0.95;
''')
color = node(u.MaterialExpressionConstant3Vector, -300, -120)
color.set_editor_property('constant', u.LinearColor(0.62, 0.006, 0.014, 1))
assert lib.connect_material_expressions(uv, '', mask, 'UV')
assert lib.connect_material_property(color, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
assert lib.connect_material_property(mask, '', u.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
assert u.EditorAssetLibrary.save_loaded_asset(mat, False)
print('DAMAGE_VIGNETTE_MATERIAL_SAVED', mat.get_path_name())
