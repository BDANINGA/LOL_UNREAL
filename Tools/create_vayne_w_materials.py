import unreal as u
lib=u.MaterialEditingLibrary
for i in range(1,4):
 name='M_VayneSilverBolts'+str(i); folder='/Game/VFX/Vayne'
 path=folder+'/'+name
 if u.EditorAssetLibrary.does_asset_exist(path):
  print('EXISTS',path);continue
 mat=u.AssetToolsHelpers.get_asset_tools().create_asset(name,folder,u.Material,u.MaterialFactoryNew())
 assert mat
 mat.set_editor_property('blend_mode',u.BlendMode.BLEND_ADDITIVE)
 mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_UNLIT)
 mat.set_editor_property('two_sided',True)
 tex=lib.create_material_expression(mat,u.MaterialExpressionTextureSampleParameter2D,-600,0)
 tex.set_editor_property('parameter_name','RingTexture')
 tex.set_editor_property('texture',u.load_asset('/Game/Level/vain/vain_tex/vayne_base_w_ring_'+str(i)))
 fade=lib.create_material_expression(mat,u.MaterialExpressionScalarParameter,-600,250)
 fade.set_editor_property('parameter_name','Brightness');fade.set_editor_property('default_value',3.)
 mul=lib.create_material_expression(mat,u.MaterialExpressionMultiply,-200,0)
 lib.connect_material_expressions(tex,'RGB',mul,'A');lib.connect_material_expressions(fade,'',mul,'B')
 lib.connect_material_property(mul,'',u.MaterialProperty.MP_EMISSIVE_COLOR)
 lib.connect_material_property(tex,'A',u.MaterialProperty.MP_OPACITY)
 lib.recompile_material(mat);u.EditorAssetLibrary.save_loaded_asset(mat)
 print('CREATED',mat.get_path_name())
