"""Rebuild the two unlit materials used by Lee Sin's Sonic Wave and its mark."""
import unreal as u

directory = '/Game/VFX/LeeSin'
library = u.MaterialEditingLibrary


def build(name, shader):
    path = directory + '/' + name
    if u.EditorAssetLibrary.does_asset_exist(path):
        material = u.load_asset(path)
    else:
        material = u.AssetToolsHelpers.get_asset_tools().create_asset(name, directory, u.Material, u.MaterialFactoryNew())
    material.set_editor_property('blend_mode', u.BlendMode.BLEND_TRANSLUCENT)
    material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property('two_sided', True)
    library.delete_all_material_expressions(material)
    shape = library.create_material_expression(material, u.MaterialExpressionCustom, -300, 0)
    inputs = []
    for name in ['UV', 'Clock', 'StartTime']:
        param = u.CustomInput()
        param.set_editor_property('input_name', name)
        inputs.append(param)
    shape.set_editor_property('inputs', inputs)
    shape.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT4)
    shape.set_editor_property('code', shader)
    uv = library.create_material_expression(material, u.MaterialExpressionTextureCoordinate, -650, 0)
    clock = library.create_material_expression(material, u.MaterialExpressionTime, -650, 140)
    start = library.create_material_expression(material, u.MaterialExpressionScalarParameter, -650, 280)
    start.set_editor_property('parameter_name', 'StartTime')
    for node, pin in [(uv, 'UV'), (clock, 'Clock'), (start, 'StartTime')]:
        assert library.connect_material_expressions(node, '', shape, pin)
    for alpha, prop in [(False, u.MaterialProperty.MP_EMISSIVE_COLOR), (True, u.MaterialProperty.MP_OPACITY)]:
        mask = library.create_material_expression(material, u.MaterialExpressionComponentMask, 0, 160 if alpha else 0)
        for channel in ['r', 'g', 'b']:
            mask.set_editor_property(channel, not alpha)
        mask.set_editor_property('a', alpha)
        assert library.connect_material_expressions(shape, '', mask, '')
        assert library.connect_material_property(mask, '', prop)
    library.recompile_material(material)
    assert u.EditorAssetLibrary.save_loaded_asset(material, False)


build('M_LeeSinSonicWave', r'''
float2 p = UV * 2 - 1;
float t = Clock - StartTime;
// A small white strike at the front with two curled pressure fronts around it.
float2 head = float2((p.x-.46)*2.2,p.y);
float radial = length(head);
float core = exp(-dot(head*float2(4,6),head*float2(4,6)));
float lens = exp(-pow((radial-.41)/.065,2))*smoothstep(-.28,.13,head.x);
float echo = exp(-pow((radial-.65)/.035,2))*smoothstep(-.02,.32,head.x);
float tail = saturate((.46-p.x)/1.43);
float envelope = smoothstep(-1,-.77,p.x)*(1-smoothstep(.32,.53,p.x));
float width = (.09+.30*sin(tail*3.14))*(1-.7*tail);
float curve = .095*sin(tail*7-t*13)*sin(tail*3.14);
float strandA = exp(-pow((p.y-curve-width*sin(tail*6+.6))/.059,2));
float strandB = exp(-pow((p.y-curve+width*sin(tail*6+.6))/.045,2));
float haze = exp(-pow((p.y-curve)/(.11+width),2));
float ribbon = envelope*(strandA+.65*strandB);
float strength = saturate(core+lens+.6*echo+ribbon+.22*haze*envelope);
float3 color = float3(.025,.27,.43)+float3(.14,.55,.56)*saturate(ribbon+lens)
             +float3(.7,.82,.77)*saturate(core+.6*lens+.22*ribbon);
return float4(color,strength);
''')

build('M_LeeSinResonanceMark', r'''
float2 p = UV*2-1;
float t = max(0,Clock-StartTime);
float r = length(p);
float a = atan2(p.y,p.x);
// Three tapered echo arcs orbit a steady ivory diamond. A single expanding
// ring acknowledges the hit before the quieter, persistent glyph takes over.
float phase = frac((a+.55*t)*3/6.2831853+.5);
float arcWidth = lerp(.075,.018,phase);
float arc = exp(-pow((r-(.57+.09*phase))/arcWidth,2))
          * smoothstep(.02,.14,phase)*(1-smoothstep(.76,.95,phase));
float diamond = abs(p.x)*1.23+abs(p.y)*.78;
float seal = exp(-pow((diamond-.245)/.041,2));
float center = (1-smoothstep(.018,.085,diamond))*(.8+.2*sin(6*t));
float flash = exp(-pow((r-lerp(.18,.91,saturate(t/.33)))/.046,2))*(1-smoothstep(.06,.34,t));
float ink = max(arc,max(seal,center));
float halo = .36*exp(-r*r/.19);
float3 color = lerp(float3(.012,.075,.12),float3(.29,.85,.94),saturate(arc+flash))
             + float3(1.1,.95,.56)*max(seal,center);
return float4(color,saturate(ink+flash+halo));
''')
