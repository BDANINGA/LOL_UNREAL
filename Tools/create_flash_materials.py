"""Flash-only angular masks. Shared hit, Exhaust and Ezreal E materials are untouched."""
import unreal as u
folder = '/Game/VFX/SummonerSpells'
lib = u.MaterialEditingLibrary
shapes = {
    'M_FlashStar': '''
float2 p = UV*2-1;
float2 a = float2(dot(p,float2(.83,.56)),dot(p,float2(-.56,.83)));
float longRay = pow(saturate(1-abs(a.x)),.65)*exp(-abs(a.y)*65);
float shortRay = pow(saturate(1-abs(a.y)*1.45),1.3)*exp(-abs(a.x)*95);
float core = exp(-(abs(a.x)+abs(a.y))*24);
return saturate(longRay+shortRay+core);
''',
    'M_FlashSlash': '''
float2 p = UV*2-1;
float x = p.x*.83+p.y*.56;
float y = -p.x*.56+p.y*.83;
float kink = .11*saturate(1-abs(x)*4);
float width = .012+.025*saturate(1-abs(x));
float blade = saturate(1-abs(y-kink)/width)*pow(saturate(1-abs(x)),.45);
float echo = exp(-abs(y+.12-kink)*95)*saturate(1-abs(x)*1.65)*.35;
return saturate(blade+echo);
''',
    'M_FlashChip': '''
float2 p=abs(UV*2-1);
float diamond=saturate(1-p.x-p.y);
return smoothstep(0,.2,diamond);
''',
}
for name, code in shapes.items():
    path=folder+'/'+name
    mat=(u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else
         u.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,u.Material,u.MaterialFactoryNew()))
    mat.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE)
    mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property('two_sided',True)
    lib.set_material_usage(mat,u.MaterialUsage.MATUSAGE_PARTICLE_SPRITES)
    counts={}
    def node(cls,x,y):
        i=counts.get(cls.__name__,0);counts[cls.__name__]=i+1
        return u.find_object(mat,cls.__name__+'_'+str(i)) or lib.create_material_expression(mat,cls,x,y)
    uv=node(u.MaterialExpressionTextureCoordinate,-700,180)
    shape=node(u.MaterialExpressionCustom,-480,180)
    shape.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
    inp=u.CustomInput();inp.set_editor_property('input_name','UV')
    shape.set_editor_property('inputs',[inp]);shape.set_editor_property('code',code)
    assert lib.connect_material_expressions(uv,'',shape,'UV')
    color=node(u.MaterialExpressionParticleColor,-700,-100)
    glow=node(u.MaterialExpressionMultiply,-220,-100);glow.set_editor_property('const_b',4.0)
    assert lib.connect_material_expressions(color,'RGB',glow,'A')
    alpha=node(u.MaterialExpressionMultiply,-220,180)
    assert lib.connect_material_expressions(shape,'',alpha,'A')
    assert lib.connect_material_expressions(color,'A',alpha,'B')
    assert lib.connect_material_property(glow,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
    assert lib.connect_material_property(alpha,'',u.MaterialProperty.MP_OPACITY)
    lib.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
    print('FLASH_MATERIAL_SAVED',path)
