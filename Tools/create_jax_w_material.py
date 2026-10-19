"""Create a separate W weapon material; retain the original weapon texture/shading."""
import unreal as u

lib = u.MaterialEditingLibrary
path = '/Game/VFX/Jax/M_JaxEmpowerWeapon'
mat = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.EditorAssetLibrary.duplicate_asset(
    '/Game/Level/jax/jax_mat/m_jax_weapon', path)
assert mat
lib.set_material_usage(mat, u.MaterialUsage.MATUSAGE_SKELETAL_MESH)
counts = {}

def node(cls, x, y):
    index = counts.get(cls.__name__, 0)
    counts[cls.__name__] = index + 1
    return u.find_object(mat, cls.__name__ + '_' + str(index)) or lib.create_material_expression(mat, cls, x, y)

texture = lib.get_material_property_input_node(mat, u.MaterialProperty.MP_BASE_COLOR)
assert texture
strength = node(u.MaterialExpressionScalarParameter, -700, 450)
strength.set_editor_property('parameter_name', 'EmpowerStrength')
strength.set_editor_property('default_value', 0.0)
fresnel = node(u.MaterialExpressionFresnel, -700, 650)
fresnel.set_editor_property('exponent', 2.2)
fresnel.set_editor_property('base_reflect_fraction', 0.1)
glow = node(u.MaterialExpressionCustom, -350, 400)
glow.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for name in ['Tex', 'Rim', 'Strength']:
    entry = u.CustomInput()
    entry.set_editor_property('input_name', name)
    inputs.append(entry)
glow.set_editor_property('inputs', inputs)
glow.set_editor_property('code', '''
// Retain engraved texture detail under a warm amber body and pale gold rim.
float detail = dot(Tex, float3(.299,.587,.114));
float3 body = float3(1.0,.28,.025) * (.18 + detail * 1.2);
float3 edge = float3(1.0,.65,.18) * Rim * 1.1;
return (body + edge) * Strength * 6.0;
''')
assert lib.connect_material_expressions(texture, 'RGB', glow, 'Tex')
assert lib.connect_material_expressions(fresnel, '', glow, 'Rim')
assert lib.connect_material_expressions(strength, '', glow, 'Strength')
assert lib.connect_material_property(glow, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(mat)
assert u.EditorAssetLibrary.save_loaded_asset(mat, False)
print('JAX_W_WEAPON_MATERIAL_READY')

path = '/Game/VFX/Jax/M_JaxEmpowerHalo'
mat = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(
    'M_JaxEmpowerHalo', '/Game/VFX/Jax', u.Material, u.MaterialFactoryNew())
mat.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided', True)
counts = {}
uv = node(u.MaterialExpressionTextureCoordinate, -700, 0)
halo = node(u.MaterialExpressionCustom, -400, 0)
halo.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
entry = u.CustomInput(); entry.set_editor_property('input_name', 'UV')
halo.set_editor_property('inputs', [entry])
halo.set_editor_property('code', '''
float2 p = UV * 2 - 1;
float r = length(p);
float falloff = pow(saturate(1-r), 2.0);
float core = exp(-r*r*75.0);
float rays = exp(-abs(p.x)*36.0) * exp(-abs(p.y)*5.0)
           + exp(-abs(p.y)*36.0) * exp(-abs(p.x)*5.0);
return (float3(1,.26,.018)*falloff*1.2
      + float3(1,.68,.22)*(core*.7+rays*.22)) * 3.0;
''')
assert lib.connect_material_expressions(uv, '', halo, 'UV')
assert lib.connect_material_property(halo, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
opacity = node(u.MaterialExpressionConstant, -400, 220)
opacity.set_editor_property('r', 1.0)
assert lib.connect_material_property(opacity, '', u.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
assert u.EditorAssetLibrary.save_loaded_asset(mat, False)
print('JAX_W_HALO_MATERIAL_READY')
