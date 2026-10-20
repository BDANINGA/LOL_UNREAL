"""Persistent ultimate armor shell and compact ground crest; no shared materials edited."""
import unreal as u
lib=u.MaterialEditingLibrary
shaders={
'M_JaxRArmor': '''
float rim=pow(saturate(Facing),2.4);
float height=sin(UV.y*3.14159265);
float columns=pow(saturate(cos(UV.x*6.2831853*10+Age*.65)),14);
float flowing=pow(saturate(sin(UV.y*32-Age*3)),16);
float pulse=1+.12*sin(Age*4);
float3 gold=float3(1,.53,.10);
float3 violet=float3(.30,.055,.60);
return (gold*(rim*.9+columns*rim*.9+flowing*rim*.32)+violet*rim*.3)*height*Fade*pulse*3.5;
''',
'M_JaxRCrest': '''
float2 p=UV*2-1;float r=length(p);float a=atan2(p.y,p.x);
float rim=exp(-pow((r-.82)/.022,2));
float inner=exp(-pow((r-.70)/.009,2))*.45;
float segment=pow(saturate(cos(a*6)),24)*smoothstep(.57,.63,r)*(1-smoothstep(.76,.79,r));
float orbit=pow(saturate(cos(a*3-Age*1.4)),12)*exp(-pow((r-.90)/.014,2));
float glow=exp(-pow((r-.76)/.14,2))*.06;
return (float3(1,.58,.12)*(rim+inner+segment*.85+glow)+float3(.65,.26,1)*orbit*.65)*Fade*2.4;
'''}
for name,code in shaders.items():
    path='/Game/VFX/Jax/'+name
    mat=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,'/Game/VFX/Jax',u.Material,u.MaterialFactoryNew())
    mat.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided',name=='M_JaxRCrest')
    counts={}
    def node(cls,x,y):
        i=counts.get(cls.__name__,0);counts[cls.__name__]=i+1
        return u.find_object(mat,cls.__name__+'_'+str(i)) or lib.create_material_expression(mat,cls,x,y)
    uv=node(u.MaterialExpressionTextureCoordinate,-700,-200)
    custom=node(u.MaterialExpressionCustom,-150,0)
    custom.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs=[]
    for key in ['UV','Age','Fade','Facing']:
        inp=u.CustomInput();inp.set_editor_property('input_name',key);inputs.append(inp)
    custom.set_editor_property('inputs',inputs);custom.set_editor_property('code',code)
    assert lib.connect_material_expressions(uv,'',custom,'UV')
    for i,key in enumerate(['Age','Fade']):
        param=node(u.MaterialExpressionScalarParameter,-700,i*150)
        param.set_editor_property('parameter_name',key);param.set_editor_property('default_value',.1 if key=='Age' else 1.)
        assert lib.connect_material_expressions(param,'',custom,key)
    facing=node(u.MaterialExpressionFresnel,-700,350)
    facing.set_editor_property('exponent',1.)
    facing.set_editor_property('base_reflect_fraction',0.)
    assert lib.connect_material_expressions(facing,'',custom,'Facing')
    assert lib.connect_material_property(custom,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    opacity=node(u.MaterialExpressionConstant,-150,300);opacity.set_editor_property('r',1.)
    assert lib.connect_material_property(opacity,'',u.MaterialProperty.MP_OPACITY)
    lib.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
    print('JAX_R_MATERIAL_READY',path)
