import unreal as u
lib=u.MaterialEditingLibrary
folder='/Game/VFX/Vayne';name='M_VayneTumbleWing';path=folder+'/'+name
mat=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,u.Material,u.MaterialFactoryNew())
lib.delete_all_material_expressions(mat)
mat.set_editor_property('blend_mode',u.BlendMode.BLEND_TRANSLUCENT)
mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
mat.set_editor_property('two_sided',True)
def node(c,x,y):return lib.create_material_expression(mat,c,x,y)
uv=node(u.MaterialExpressionTextureCoordinate,-600,0)
shape=node(u.MaterialExpressionCustom,-300,0)
i=u.CustomInput();i.set_editor_property('input_name','UV');shape.set_editor_property('inputs',[i]);shape.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT1)
shape.set_editor_property('code','''
float2 p=UV*2-1;float x=abs(p.x);
float top=.22+.38*sin(x*3.14159);
float bottom=-.35+.35*x+.16*cos(x*9.42477);
float wing=smoothstep(bottom-.035,bottom+.035,p.y)*(1-smoothstep(top-.035,top+.035,p.y))*(1-smoothstep(.86,.98,x));
float body=exp(-p.x*p.x*160)*exp(-p.y*p.y*9);
return saturate(wing+body);
''')
lib.connect_material_expressions(uv,'',shape,'UV')
fade=node(u.MaterialExpressionScalarParameter,-600,250);fade.set_editor_property('parameter_name','Fade');fade.set_editor_property('default_value',1.)
a=node(u.MaterialExpressionMultiply,-60,150);lib.connect_material_expressions(shape,'',a,'A');lib.connect_material_expressions(fade,'',a,'B');lib.connect_material_property(a,'',u.MaterialProperty.MP_OPACITY)
c=node(u.MaterialExpressionVectorParameter,-300,-250);c.set_editor_property('parameter_name','Tint');c.set_editor_property('default_value',u.LinearColor(.18,.015,.38,1))
lib.connect_material_property(c,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
lib.recompile_material(mat);assert u.EditorAssetLibrary.save_loaded_asset(mat,False)
print('VAYNE_TUMBLE_MATERIAL_READY')
