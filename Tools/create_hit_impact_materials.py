"""Standalone hit-effect materials. No game actors, maps or attack logic are modified."""
import unreal as u

folder = '/Game/VFX/HitImpact'
lib = u.MaterialEditingLibrary
shapes = {
    'M_HitFlash': r'''
        float2 p = UV * 2 - 1;
        float r = length(p);
        float glow = pow(saturate(1-r), 3.0);
        float cross = exp(-abs(p.x)*38)*pow(saturate(1-abs(p.y)),2)
                    + exp(-abs(p.y)*38)*pow(saturate(1-abs(p.x)),2);
        return saturate(glow*1.6 + cross*0.65);
    ''',
    'M_HitSpark': r'''
        float2 p = UV * 2 - 1;
        return pow(saturate(1-abs(p.x)),2.0) * pow(saturate(1-abs(p.y)),0.65);
    ''',
    'M_HitRing': r'''
        float2 p = UV * 2 - 1;
        float r = length(p);
        float ring = exp(-pow((r-0.72)/0.045,2));
        return ring * saturate((1-r)*12);
    ''',
}

for name, code in shapes.items():
    path = folder + '/' + name
    mat = (u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else
           u.AssetToolsHelpers.get_asset_tools().create_asset(name, folder, u.Material, u.MaterialFactoryNew()))
    u.get_editor_subsystem(u.AssetEditorSubsystem).close_all_editors_for_asset(mat)
    mat.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    lib.set_material_usage(mat, u.MaterialUsage.MATUSAGE_PARTICLE_SPRITES)
    counts = {}
    def node(cls, x, y):
        index = counts.get(cls.__name__, 0)
        counts[cls.__name__] = index + 1
        return u.find_object(mat, cls.__name__+'_'+str(index)) or lib.create_material_expression(mat, cls, x, y)
    uv = node(u.MaterialExpressionTextureCoordinate, -700, 180)
    shape = node(u.MaterialExpressionCustom, -480, 180)
    shape.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT1)
    item = u.CustomInput(); item.set_editor_property('input_name', 'UV')
    shape.set_editor_property('inputs', [item])
    shape.set_editor_property('code', code)
    assert lib.connect_material_expressions(uv, '', shape, 'UV')
    color = node(u.MaterialExpressionParticleColor, -700, -100)
    glow = node(u.MaterialExpressionMultiply, -220, -100)
    glow.set_editor_property('const_b', 3.5)
    assert lib.connect_material_expressions(color, 'RGB', glow, 'A')
    alpha = node(u.MaterialExpressionMultiply, -220, 180)
    assert lib.connect_material_expressions(shape, '', alpha, 'A')
    assert lib.connect_material_expressions(color, 'A', alpha, 'B')
    assert lib.connect_material_property(glow, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    assert lib.connect_material_property(alpha, '', u.MaterialProperty.MP_OPACITY)
    lib.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat, False)
    print('HIT_MATERIAL_SAVED', path)
