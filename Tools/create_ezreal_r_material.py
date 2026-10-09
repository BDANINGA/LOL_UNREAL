"""Author the R wave material in Unreal; no external textures or plugins required."""
import unreal as u

path = '/Game/Level/ezreal/FX/R'
name = 'M_EzrealRWave'
lib = u.MaterialEditingLibrary
mat = (u.load_asset(path + '/' + name) if u.EditorAssetLibrary.does_asset_exist(path + '/' + name)
       else u.AssetToolsHelpers.get_asset_tools().create_asset(name, path, u.Material, u.MaterialFactoryNew()))
# Constructor-referenced materials may have rooted expressions in UE 5.7.
# Reuse the graph instead of deleting those expressions when tuning the effect.
u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(mat)
mat.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided', True)

node_counts = {}
def node(cls, x, y):
    index = node_counts.get(cls.__name__, 0)
    node_counts[cls.__name__] = index + 1
    existing = u.find_object(mat, cls.__name__ + '_' + str(index))
    return existing or lib.create_material_expression(mat, cls, x, y)

world = node(u.MaterialExpressionWorldPosition, -800, -200)
local = node(u.MaterialExpressionTransformPosition, -600, -200)
local.set_editor_property('transform_source_type', u.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_WORLD)
local.set_editor_property('transform_type', u.MaterialPositionTransformSource.TRANSFORMPOSSOURCE_LOCAL)
assert lib.connect_material_expressions(world, '', local, '')
time = node(u.MaterialExpressionTime, -600, 0)
fade = node(u.MaterialExpressionScalarParameter, -600, 150)
fade.set_editor_property('parameter_name', 'WaveFade')
fade.set_editor_property('default_value', 1.0)
glow = node(u.MaterialExpressionScalarParameter, -600, 300)
glow.set_editor_property('parameter_name', 'WaveIntensity')
glow.set_editor_property('default_value', 1.0)
shader = node(u.MaterialExpressionCustom, -250, -100)
shader.set_editor_property('description', 'Forward +X crescent, flowing arcane filaments and tapered wake')
shader.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
inputs = []
for name in ['P', 'T', 'Fade', 'Intensity']:
    item = u.CustomInput()
    item.set_editor_property('input_name', name)
    inputs.append(item)
shader.set_editor_property('inputs', inputs)
shader.set_editor_property('code', r'''
float x = P.x / 50.0;
float y = P.y / 50.0;
float span = saturate(1.0 - pow(abs(y), 5.0));
float edge = 0.34 - 0.88 * y * y;
float d = edge - x;
float taper = 0.022 + 0.095 * span;
float core = exp(-pow(d / taper, 2.0) * 3.0);
float rim = exp(-pow((d + 0.021) / 0.018, 2.0));
float body = exp(-max(d, 0.0) * 7.0) * smoothstep(-0.045, 0.03, d);
float wake = exp(-max(d, 0.0) * 3.7) * smoothstep(0.015, 0.15, d);
float flow = sin(y * 43.0 + d * 27.0 - T * 14.0 + sin(y * 17.0 + T * 3.0));
float breakup = 0.5 + 0.5 * sin(y * 31.0 - d * 67.0 + T * 9.0);
float filaments = pow(saturate(flow), 9.0) * smoothstep(0.3, 0.9, breakup);
float seams = exp(-pow((d - 0.19 - 0.025 * sin(y * 24.0 - T * 10.0)) / 0.017, 2.0));
float ends = smoothstep(0.0, 0.14, span);
float backClip = smoothstep(-1.0, -0.84, x);
float3 ivory = float3(1.0, 0.87, 0.42);
float3 gold = float3(1.0, 0.44, 0.035);
float3 cyan = float3(0.04, 0.56, 0.95);
float3 color = ivory * rim * 0.85
    + gold * (core * 3.2 + body * 1.8)
    + cyan * (wake * (0.12 + filaments * 0.85) + seams * 0.18);
return color * ends * backClip * Fade * Intensity;
''')
for source, name in [(local, 'P'), (time, 'T'), (fade, 'Fade'), (glow, 'Intensity')]:
    assert lib.connect_material_expressions(source, '', shader, name)
assert lib.connect_material_property(shader, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
opacity = node(u.MaterialExpressionConstant, -100, 400)
opacity.set_editor_property('r', 1.0)
assert lib.connect_material_property(opacity, '', u.MaterialProperty.MP_OPACITY)
lib.recompile_material(mat)
assert u.EditorAssetLibrary.save_loaded_asset(mat, False)
print('R_WAVE_MATERIAL_SAVED', mat.get_path_name())
