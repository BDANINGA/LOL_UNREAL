"""Author texture-free arcane recall materials; only /Game/VFX/Recall is saved."""
import unreal as u

folder = '/Game/VFX/Recall'
lib = u.MaterialEditingLibrary
shaders = {
    'M_RecallSigil': r'''
float2 p = UV * 2.0 - 1.0;
float r = length(p);
float a = atan2(p.y, p.x);
float aa = max(fwidth(r), 0.002);
float reveal = smoothstep(r - 0.09, r + 0.04, saturate(Age / 0.65) * 1.1);
float outer = 1.0 - smoothstep(0.006, 0.006 + aa, abs(r - 0.88));
float rail = 1.0 - smoothstep(0.003, 0.003 + aa, abs(r - 0.79));
float inner = 1.0 - smoothstep(0.006, 0.006 + aa, abs(r - 0.54));
float arc = (1.0 - smoothstep(0.008, 0.008 + aa, abs(r - 0.66)))
    * smoothstep(-0.25, -0.1, cos((a + Age * 0.20) * 6.0));
// Each angular cell contains an angular, rune-like stroke, never a solid disc.
float cell = frac((a - Age * 0.10) * 3.8197186 + 0.5);
float gx = (cell - 0.5) * 2.0;
float gy = (r - 0.835) / 0.030;
float runeStem = 1.0 - smoothstep(0.10, 0.24, abs(gx + 0.24));
float runeArm = 1.0 - smoothstep(0.10, 0.24, abs(gy - gx * 1.5));
float runeCross = (1.0 - smoothstep(0.10, 0.24, abs(gy + gx * 1.5))) * step(0.0, gx);
float runes = max(runeStem, max(runeArm, runeCross)) * (1.0 - smoothstep(0.72, 1.0, abs(gy)));
float b = a + Age * 0.15;
float sector = (frac(b / 1.04719755 + 0.5) - 0.5) * 1.04719755;
float hexRadius = r * cos(sector);
float hex = 1.0 - smoothstep(0.004, 0.004 + aa, abs(hexRadius - 0.43));
float c = a - Age * 0.22;
float spoke = pow(saturate(cos(c * 6.0)), 110.0) * smoothstep(0.23, 0.28, r) * (1.0 - smoothstep(0.51, 0.55, r));
float diamondA = (frac((a + Age * 0.08) / 1.5707963 + 0.5) - 0.5) * 1.5707963;
float2 dp = float2(r * sin(diamondA), r * cos(diamondA) - 0.94);
float diamond = 1.0 - smoothstep(0.004, 0.004 + aa, abs(abs(dp.x) + abs(dp.y) - 0.030));
float pulse = 0.84 + 0.16 * sin(Age * 2.0 + r * 11.0);
float aura = exp(-pow((r - 0.74) / 0.16, 2.0)) * 0.045;
float3 blue = float3(0.04, 0.52, 1.0);
float3 cyan = float3(0.25, 0.90, 1.0);
float3 ivory = float3(0.70, 0.93, 1.0);
float3 color = cyan * (outer + rail * 0.55 + runes * 0.85)
    + blue * (arc * 0.9 + hex * 0.75 + aura)
    + ivory * (inner * 0.7 + diamond * 1.2 + spoke * 0.65);
return color * reveal * pulse * Fade * (1.35 + Charge * 1.7);
''',
    'M_RecallColumn': r'''
float x = UV.x * 2.0 - 1.0;
float y = UV.y;
float ends = smoothstep(0.0, 0.16, y) * (1.0 - smoothstep(0.55, 1.0, y));
float soft = exp(-x * x * 10.0) * 0.10;
float flow = x + 0.10 * sin(y * 8.0 - Age * 1.6);
float ribbons = exp(-pow((flow - 0.51) / 0.025, 2.0))
    + exp(-pow((flow + 0.51) / 0.025, 2.0));
float strands = pow(saturate(cos(x * 24.0 + y * 5.0 - Age)), 28.0) * 0.075;
float3 color = float3(0.04, 0.5, 1.0) * soft + float3(0.2, 0.85, 1.0) * (ribbons * 0.35 + strands);
return color * ends * Fade * (0.25 + Charge * 1.65);
''',
    'M_RecallMote': r'''
float2 p = abs(UV * 2.0 - 1.0);
float core = pow(saturate(1.0 - p.x - p.y), 2.0);
float ray = exp(-p.x * 48.0) * pow(saturate(1.0 - p.y), 3.0)
    + exp(-p.y * 48.0) * pow(saturate(1.0 - p.x), 3.0);
return float3(0.24, 0.8, 1.0) * (core + ray * 0.35) * Fade * (2.0 + Charge);
'''
}

for name, code in shaders.items():
    path = folder + '/' + name
    mat = (u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else
           u.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, u.Material, u.MaterialFactoryNew()))
    u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(mat)
    mat.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    counts = {}

    def node(cls, x, y):
        index = counts.get(cls.__name__, 0)
        counts[cls.__name__] = index + 1
        return u.find_object(mat, cls.__name__ + '_' + str(index)) or lib.create_material_expression(mat, cls, x, y)

    uv = node(u.MaterialExpressionTextureCoordinate, -700, -200)
    parameters = {}
    for i, (key, default) in enumerate([('Age', 2.0), ('Fade', 1.0), ('Charge', 0.5)]):
        param = node(u.MaterialExpressionScalarParameter, -700, i * 130)
        param.set_editor_property('parameter_name', key)
        param.set_editor_property('default_value', default)
        parameters[key] = param
    custom = node(u.MaterialExpressionCustom, -300, -100)
    custom.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs = []
    for key in ['UV', 'Age', 'Fade', 'Charge']:
        item = u.CustomInput()
        item.set_editor_property('input_name', key)
        inputs.append(item)
    custom.set_editor_property('inputs', inputs)
    custom.set_editor_property('code', code)
    assert lib.connect_material_expressions(uv, '', custom, 'UV')
    for key, param in parameters.items():
        assert lib.connect_material_expressions(param, '', custom, key)
    assert lib.connect_material_property(custom, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity = node(u.MaterialExpressionConstant, -300, 500)
    opacity.set_editor_property('r', 1.0)
    assert lib.connect_material_property(opacity, '', u.MaterialProperty.MP_OPACITY)
    lib.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat, False)
    print('RECALL_MATERIAL_SAVED', path)
