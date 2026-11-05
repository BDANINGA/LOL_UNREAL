"""Procedural water sheets and foam droplets; no external textures required."""
import unreal as u
lib = u.MaterialEditingLibrary
shapes = {
    'M_FizzQWaterSheet': '''
float2 p=UV*2-1;
float taper=pow(saturate(1-p.y*p.y),.65);
float crest=.50*(1-p.y*p.y)-.25;
float d=p.x-crest;
float body=exp(-pow((d+.12)/(.07+.16*taper),2))*taper;
float rim=exp(-pow(d/.028,2))*taper;
float ribs=pow(saturate(sin(p.y*37+p.x*14)),10)*body*.25;
float echo=exp(-pow((d+.34)/.025,2))*taper*.3;
return float4(float3(.025,.42,.65)*body+float3(.65,.95,1)*(rim+ribs+echo),saturate(body*.6+rim+echo));
''',
    'M_FizzQDroplet': '''
float2 p=UV*2-1;
float r=length(float2(p.x/(.56-.16*p.y),p.y));
float mask=1-smoothstep(.72,1,r);
float highlight=exp(-dot(p-float2(-.13,.23),p-float2(-.13,.23))*28);
return float4(lerp(float3(.05,.55,.85),float3(.85,1,1),highlight+.3),mask);
''',
    'M_FizzQFoam': '''
float2 p=UV*2-1;
float r=length(p);
float a=atan2(p.y,p.x);
float edge=.62+.07*sin(a*7)+.04*cos(a*11);
float ring=exp(-pow((r-edge)/.065,2));
float breaks=smoothstep(-.4,.6,sin(a*9+r*19));
float soft=exp(-r*r*7)*.13;
return float4(float3(.5,.88,1),saturate(ring*breaks*.65+soft)*(1-smoothstep(.85,1,r)));
'''
}
for name, code in shapes.items():
    path='/Game/VFX/Fizz/'+name
    mat=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,'/Game/VFX/Fizz',u.Material,u.MaterialFactoryNew())
    mat.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided',True)
    lib.set_material_usage(mat,u.MaterialUsage.MATUSAGE_PARTICLE_SPRITES)
    lib.delete_all_material_expressions(mat)
    uv=lib.create_material_expression(mat,u.MaterialExpressionTextureCoordinate,-800,0)
    shape=lib.create_material_expression(mat,u.MaterialExpressionCustom,-580,0)
    shape.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT4)
    inp=u.CustomInput();inp.set_editor_property('input_name','UV');shape.set_editor_property('inputs',[inp]);shape.set_editor_property('code',code)
    assert lib.connect_material_expressions(uv,'',shape,'UV')
    color=lib.create_material_expression(mat,u.MaterialExpressionParticleColor,-600,250)
    rgb=lib.create_material_expression(mat,u.MaterialExpressionComponentMask,-350,-100)
    rgb.set_editor_property('r',True);rgb.set_editor_property('g',True);rgb.set_editor_property('b',True)
    assert lib.connect_material_expressions(shape,'',rgb,'')
    alpha=lib.create_material_expression(mat,u.MaterialExpressionComponentMask,-350,100)
    for channel in ['r','g','b']: alpha.set_editor_property(channel,False)
    alpha.set_editor_property('a',True)
    assert lib.connect_material_expressions(shape,'',alpha,'')
    for source,pin,target in [(rgb,'RGB',u.MaterialProperty.MP_EMISSIVE_COLOR),(alpha,'A',u.MaterialProperty.MP_OPACITY)]:
        mul=lib.create_material_expression(mat,u.MaterialExpressionMultiply,-100,0 if pin=='RGB' else 200)
        assert lib.connect_material_expressions(source,'',mul,'A');assert lib.connect_material_expressions(color,pin,mul,'B')
        assert lib.connect_material_property(mul,'',target)
    lib.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
    print('FIZZ_WATER_MATERIAL',path)
