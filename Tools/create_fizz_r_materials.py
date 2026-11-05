"""Author procedural fin shading, directional wake and timed shark warning."""
import unreal as u
lib=u.MaterialEditingLibrary

def make(name,code,opaque=False):
    path='/Game/VFX/Fizz/'+name
    mat=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,'/Game/VFX/Fizz',u.Material,u.MaterialFactoryNew())
    mat.set_editor_property('blend_mode',u.BlendMode.BLEND_OPAQUE if opaque else u.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided',True)
    lib.delete_all_material_expressions(mat)
    uv=lib.create_material_expression(mat,u.MaterialExpressionTextureCoordinate,-700,0)
    shape=lib.create_material_expression(mat,u.MaterialExpressionCustom,-350,0)
    shape.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT4)
    inputs=[]
    for name in ['UV','Age','Progress']:
        i=u.CustomInput();i.set_editor_property('input_name',name);inputs.append(i)
    shape.set_editor_property('inputs',inputs);shape.set_editor_property('code',code)
    assert lib.connect_material_expressions(uv,'',shape,'UV')
    for i,name in enumerate(['Age','Progress']):
        param=lib.create_material_expression(mat,u.MaterialExpressionScalarParameter,-700,150+i*150)
        param.set_editor_property('parameter_name',name);param.set_editor_property('default_value',0.)
        assert lib.connect_material_expressions(param,'',shape,name)
    for alpha,prop in [(False,u.MaterialProperty.MP_EMISSIVE_COLOR),(True,u.MaterialProperty.MP_OPACITY)]:
        if opaque and alpha:continue
        mask=lib.create_material_expression(mat,u.MaterialExpressionComponentMask,0,150 if alpha else -50)
        for channel in ['r','g','b']:mask.set_editor_property(channel,not alpha)
        mask.set_editor_property('a',alpha)
        assert lib.connect_material_expressions(shape,'',mask,'')
        assert lib.connect_material_property(mask,'',prop)
    lib.recompile_material(mat);assert u.EditorAssetLibrary.save_loaded_asset(mat,False)

make('M_FizzRFin',r'''
float h=saturate(UV.y); float x=UV.x*160-80;
float leading=68-104*pow(h,.85);
float rim=exp(-pow((x-leading)/6,2));
float ridge=exp(-pow((UV.x-.48+h*.12)/.12,2));
float ribs=pow(saturate(sin(UV.x*58+h*24)),18)*.07;
float3 body=lerp(float3(.018,.07,.11),float3(.08,.27,.36),h);
body+=ridge*float3(.035,.07,.09)+rim*float3(.16,.36,.42)+ribs*float3(.06,.15,.17);
return float4(body,1);
''',True)

make('M_FizzRWake',r'''
float2 p=UV*2-1; float x=p.x, y=abs(p.y);
// Tip at +X, widening broken white crests behind the fin.
float aft=saturate((.55-x)/1.5);
float width=.06+pow(aft,.8)*.78+.018*sin(x*23-Age*8)+.009*sin(x*49+Age*13);
float broken=.65+.35*sin(x*65+y*30+Age*18)*sin(y*57-Age*11);
float edge=exp(-pow((y-width)/.035,2))*broken;
float inner=exp(-pow((y-width*.64)/.025,2))*.35;
float fill=(1-smoothstep(width-.03,width+.03,y))*.22;
float fade=smoothstep(-1,-.65,x)*(1-smoothstep(.48,.7,x));
float strands=pow(saturate(sin(x*17-y*25+sin(x*13+Age*5)*1.4+Age*9)),7)*fill*.35;
float3 color=float3(.015,.24,.32)*fill+float3(.55,.88,.95)*edge+float3(.08,.48,.64)*(inner+strands);
return float4(color,saturate(edge*.85+inner*.5+fill+strands)*fade);
''')

make('M_FizzRWarning',r'''
float2 p=UV*2-1; float r=length(p), a=atan2(p.y,p.x);
float border=exp(-pow((r-.94)/.009,2));
float arc=frac(a/6.2831853+.75);
float timer=exp(-pow((r-.905)/.014,2))*(1-smoothstep(Progress,Progress+.015,arc));
float swirl=pow(saturate(sin(a*3+r*23+Age*4.8)),12);
float grain=.65+.35*sin(a*31+r*96-Age*11)*sin(a*17-r*47+Age*8);
float crest=swirl*grain*smoothstep(.15,.5,r)*(1-smoothstep(.77,.9,r));
float pool=(1-smoothstep(.88,.94,r))*.26;
float ripples=pow(saturate(sin(r*47+Age*8-a*2)),18)*pool*.65;
float pulse=(.5+.5*sin(Age*16))*smoothstep(.65,1,Progress);
float3 color=float3(.005,.065,.11)*pool+float3(.035,.3,.4)*(crest+ ripples);
color+=float3(.12,.66,.8)*border*(.65+pulse*.35)+float3(.65,.9,1)*timer;
float alpha=pool+crest*.38+ripples+border*.85+timer*.85;
return float4(color,saturate(alpha)*(1-smoothstep(.96,.99,r)));
''')
print('FIZZ_R_MATERIALS_READY')
