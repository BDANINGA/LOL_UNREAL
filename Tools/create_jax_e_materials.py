"""Procedural counter-strike range ring, spinning ribbons and release wave."""
import unreal as u
lib = u.MaterialEditingLibrary
shaders = {
    'M_JaxERange': '''
float2 p=UV*2-1; float r=length(p); float a=atan2(p.y,p.x);
float aa=max(fwidth(r),.002);
// The outer edge is exactly r=.94; component scale maps it to the damage radius.
float edge=(1-smoothstep(.932,.94,r))*smoothstep(.918,.93,r);
float ticks=pow(saturate(cos(a*24)),48)*smoothstep(.87,.89,r)*(1-smoothstep(.91,.92,r));
float inner=exp(-pow((r-.89)/.023,2))*.10;
return float3(1,.48,.10)*(edge*.65+ticks*.38+inner)*Fade*(1.2+.15*sin(Age*5));
''',
    'M_JaxESpin': '''
float2 p=UV*2-1; float r=length(p); float a=atan2(p.y,p.x);
float phase=frac((a-Age*11.0)/6.2831853);
float tail=pow(saturate(1-phase/.62),1.7);
float band=exp(-pow((r-(.75+.025*sin(a*3-Age*7)))/.045,2));
float rimLine=exp(-pow((r-.81)/.009,2));
float echoPhase=frac((a-Age*11.0+3.1415926)/6.2831853);
float echo=exp(-pow((r-.65)/.022,2))*pow(saturate(1-echoPhase/.36),2);
float dust=pow(saturate(cos(a*17-Age*8)),50)*exp(-pow((r-.87)/.018,2));
return (float3(1,.55,.12)*(band*tail*1.7+echo*.7)
      +float3(1,.88,.50)*(rimLine*tail*.8+dust*.25))*Fade*3.0;
''',
    'M_JaxERelease': '''
float2 p=UV*2-1; float r=length(p); float a=atan2(p.y,p.x);
float t=saturate(Age/.38);
float radius=lerp(.12,.94,1-pow(1-t,2));
float wave=exp(-pow((r-radius)/(.026+.035*t),2));
float echo=exp(-pow((r-radius*.82)/.012,2))*.35;
float rays=pow(saturate(cos(a*16)),36)*smoothstep(radius*.65,radius,r)*(1-smoothstep(radius,radius+.06,r));
float clip=1-smoothstep(.932,.94,r);
return (float3(1,.64,.18)*(wave+echo)+float3(1,.88,.55)*rays*.65)*clip*Fade*4;
'''
}
for name, code in shaders.items():
    path = '/Game/VFX/Jax/' + name
    mat = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(name, '/Game/VFX/Jax', u.Material, u.MaterialFactoryNew())
    mat.set_editor_property('blend_mode', u.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided', True)
    counts = {}
    def node(cls, x, y):
        i=counts.get(cls.__name__,0); counts[cls.__name__]=i+1
        return u.find_object(mat,cls.__name__+'_'+str(i)) or lib.create_material_expression(mat,cls,x,y)
    uv=node(u.MaterialExpressionTextureCoordinate,-600,-200)
    custom=node(u.MaterialExpressionCustom,-200,0)
    custom.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs=[]
    for key in ['UV','Age','Fade']:
        entry=u.CustomInput(); entry.set_editor_property('input_name',key); inputs.append(entry)
    custom.set_editor_property('inputs',inputs)
    custom.set_editor_property('code',code)
    assert lib.connect_material_expressions(uv,'',custom,'UV')
    for i,key in enumerate(['Age','Fade']):
        param=node(u.MaterialExpressionScalarParameter,-600,i*160)
        param.set_editor_property('parameter_name',key)
        param.set_editor_property('default_value',.1 if key=='Age' else 1.0)
        assert lib.connect_material_expressions(param,'',custom,key)
    assert lib.connect_material_property(custom,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity=node(u.MaterialExpressionConstant,-200,400); opacity.set_editor_property('r',1.)
    assert lib.connect_material_property(opacity,'',u.MaterialProperty.MP_OPACITY)
    lib.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
    print('JAX_E_MATERIAL_READY',path)
