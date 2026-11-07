"""Animated shallow water sheet with broken foam crests for Fizz E."""
import unreal as u
lib=u.MaterialEditingLibrary
path='/Game/VFX/Fizz/M_FizzERipple'
mat=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset('M_FizzERipple','/Game/VFX/Fizz',u.Material,u.MaterialFactoryNew())
mat.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided',True)
lib.delete_all_material_expressions(mat)
uv=lib.create_material_expression(mat,u.MaterialExpressionTextureCoordinate,-700,0)
shape=lib.create_material_expression(mat,u.MaterialExpressionCustom,-350,0)
shape.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT4)
inputs=[]
for name in ['UV','Age','Life']:
    i=u.CustomInput();i.set_editor_property('input_name',name);inputs.append(i)
shape.set_editor_property('inputs',inputs)
shape.set_editor_property('code','''
float2 p=UV*2-1; float r=length(p); float a=atan2(p.y,p.x);
float t=saturate(Age/Life);
float radius=lerp(.13,.94,1-pow(1-saturate(t/.62),2));
float wobble=.018*sin(a*13+Age*16)+.012*sin(a*21-Age*11);
float crest=exp(-pow((r-radius-wobble)/.024,2));
float echo=exp(-pow((r-radius*.78+wobble)/.016,2))*.4;
float grain=sin(a*25+r*91-Age*18)*sin(a*11-r*60);
float foam=crest*smoothstep(-.6,.3,grain);
float sheet=smoothstep(radius*.4,radius*.8,r)*(1-smoothstep(radius-.035,radius+.01,r));
float veins=pow(saturate(sin(a*19+r*55-Age*10)),14)*sheet*.16;
float fade=(1-smoothstep(.35,1,t))*(1-smoothstep(.94,.98,r));
float3 color=float3(.015,.34,.5)*sheet+float3(.35,.82,.95)*(crest*.5+echo+veins)+float3(.8,1,1)*foam;
return float4(color,saturate(sheet*.24+foam*.75+crest*.18+echo*.4+veins)*fade);
''')
assert lib.connect_material_expressions(uv,'',shape,'UV')
for i,(name,value) in enumerate([('Age',0.),('Life',.9)]):
    param=lib.create_material_expression(mat,u.MaterialExpressionScalarParameter,-700,160+i*150)
    param.set_editor_property('parameter_name',name);param.set_editor_property('default_value',value)
    assert lib.connect_material_expressions(param,'',shape,name)
for is_alpha,prop in [(False,u.MaterialProperty.MP_EMISSIVE_COLOR),(True,u.MaterialProperty.MP_OPACITY)]:
    mask=lib.create_material_expression(mat,u.MaterialExpressionComponentMask,0,150 if is_alpha else -50)
    for channel in ['r','g','b']:mask.set_editor_property(channel,not is_alpha)
    mask.set_editor_property('a',is_alpha)
    assert lib.connect_material_expressions(shape,'',mask,'')
    assert lib.connect_material_property(mask,'',prop)
lib.recompile_material(mat)
assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
print('FIZZ_E_RIPPLE_READY')
