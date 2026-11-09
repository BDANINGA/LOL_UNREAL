"""Create an editable mechanical fist and its gold, steel, energy and cable materials."""
import math
from pathlib import Path
import unreal as u

assert not u.get_editor_subsystem(u.LevelEditorSubsystem).is_in_play_in_editor()
folder = '/Game/VFX/Blitz'
source = Path(u.Paths.project_dir()).resolve().parent / 'Tools/Art/Blitz'
source.mkdir(parents=True, exist_ok=True)
vertices, faces = [], []

def face(points, material):
    start = len(vertices) + 1
    vertices.extend(points)
    for i in range(1, len(points) - 1):
        faces.append((material, (start, start + i, start + i + 1)))

def box(center, size, bevel, material):
    half = [s / 2 for s in size]
    for axis in range(3):
        a, b = (axis + 1) % 3, (axis + 2) % 3
        for sign in [-1, 1]:
            grid = [[-h, -h + bevel, h - bevel, h] for h in half]
            for i in range(3):
                for j in range(3):
                    points = []
                    for di, dj in [(0, 0), (1, 0), (1, 1), (0, 1)]:
                        p = [0., 0., 0.]
                        p[axis] = sign * half[axis]
                        p[a], p[b] = grid[a][i + di], grid[b][j + dj]
                        core = [max(-h + bevel, min(h - bevel, x)) for x, h in zip(p, half)]
                        offset = [x - c for x, c in zip(p, core)]
                        length = math.sqrt(sum(x * x for x in offset))
                        points.append(tuple(c + o * bevel / length + origin for c, o, origin in zip(core, offset, center)))
                    if sign < 0:
                        points.reverse()
                    face(points, material)

def sleeve(profile, material):
    for (x1, r1), (x2, r2) in zip(profile, profile[1:]):
        for j in range(16):
            a, b = j * math.tau / 16, (j + 1) * math.tau / 16
            face([(x1, r1*math.cos(a), r1*math.sin(a)),
                  (x1, r1*math.cos(b), r1*math.sin(b)),
                  (x2, r2*math.cos(b), r2*math.sin(b)),
                  (x2, r2*math.cos(a), r2*math.sin(a))], material)

# +X is the direction of travel. Broad knuckles and separate folded fingers read from above.
box((0, 0, 0), (36, 48, 34), 4, 'Gold')
for y in [-18, -6, 6, 18]:
    box((20, y, 5), (24, 10.5, 23), 3, 'Gold')
    box((18, y, -13), (20, 10, 13), 2.5, 'Steel')
box((1, -28, -3), (27, 14, 23), 4, 'Gold')
sleeve([(-37, 0), (-37, 17), (-34, 21), (-22, 21), (-18, 17)], 'Steel')
sleeve([(-33, 21), (-32, 23), (-28, 23), (-27, 21)], 'Gold')
sleeve([(-23, 21), (-22, 22), (-20, 22), (-19, 18)], 'Gold')
sleeve([(-36, 18), (-35, 19), (-34, 19)], 'Energy')
for x in [-10, -3, 4]:
    box((x, 0, 17.3), (3, 22, 1.5), .5, 'Energy')
for y in [-20, 20]:
    box((-12, y, 17), (5, 5, 3), 1, 'Steel')

lines = ['mtllib BlitzGrab.mtl', 'o SM_BlitzRocketGrab', 's off']
lines.extend('v %.6f %.6f %.6f' % p for p in vertices)
material_name = None
for material, indices in faces:
    if material != material_name:
        lines.append('usemtl ' + material)
        material_name = material
    lines.append('f ' + ' '.join(str(i) for i in indices))
(source / 'SM_BlitzRocketGrab.obj').write_text('\n'.join(lines) + '\n', encoding='utf8')
(source / 'BlitzGrab.mtl').write_text('newmtl Gold\nKd .5 .25 .06\nnewmtl Steel\nKd .08 .10 .12\nnewmtl Energy\nKd .04 .5 .8\n', encoding='utf8')

lib = u.MaterialEditingLibrary
assets = u.AssetToolsHelpers.get_asset_tools()

def make_material(name, color, metallic, roughness, glow=0):
    path = folder + '/' + name
    mat = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else assets.create_asset(name, folder, u.Material, u.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat)
    mat.set_editor_property('shading_model', u.MaterialShadingModel.MSM_DEFAULT_LIT)
    if name.endswith('Cable'):
        mat.set_editor_property('used_with_spline_meshes', True)
    base = lib.create_material_expression(mat, u.MaterialExpressionConstant3Vector, -400, 0)
    base.set_editor_property('constant', u.LinearColor(*color, 1))
    lib.connect_material_property(base, '', u.MaterialProperty.MP_BASE_COLOR)
    for y, value, prop in [(120, metallic, u.MaterialProperty.MP_METALLIC), (220, roughness, u.MaterialProperty.MP_ROUGHNESS)]:
        n = lib.create_material_expression(mat, u.MaterialExpressionConstant, -400, y)
        n.set_editor_property('r', value)
        lib.connect_material_property(n, '', prop)
    if glow:
        emissive = lib.create_material_expression(mat, u.MaterialExpressionConstant3Vector, -400, 320)
        emissive.set_editor_property('constant', u.LinearColor(*(c * glow for c in color), 1))
        lib.connect_material_property(emissive, '', u.MaterialProperty.MP_EMISSIVE_COLOR)
    if name.endswith('Cable'):
        uv = lib.create_material_expression(mat, u.MaterialExpressionTextureCoordinate, -800, -200)
        rib = lib.create_material_expression(mat, u.MaterialExpressionCustom, -400, -200)
        inp = u.CustomInput(); inp.set_editor_property('input_name', 'UV')
        rib.set_editor_property('inputs', [inp])
        rib.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
        rib.set_editor_property('code', 'float rib = smoothstep(.42,.48,frac(UV.y*48)); return lerp(float3(.035,.045,.055),float3(.26,.30,.32),rib);')
        lib.connect_material_expressions(uv, '', rib, 'UV')
        lib.connect_material_property(rib, '', u.MaterialProperty.MP_BASE_COLOR)
    lib.recompile_material(mat)
    assert u.EditorAssetLibrary.save_loaded_asset(mat, False)
    return mat

materials = {
    'Gold': make_material('M_BlitzGrabGold', (.48, .25, .055), .65, .34, .16),
    'Steel': make_material('M_BlitzGrabSteel', (.075, .095, .12), .75, .32, .05),
    'Energy': make_material('M_BlitzGrabEnergy', (.015, .4, .75), .15, .25, 2.5),
}
make_material('M_BlitzGrabCable', (.1, .12, .15), .65, .38, .1)
task = u.AssetImportTask()
task.set_editor_property('filename', str(source / 'SM_BlitzRocketGrab.obj'))
task.set_editor_property('destination_path', folder)
task.set_editor_property('destination_name', 'SM_BlitzRocketGrab')
task.set_editor_property('automated', True)
task.set_editor_property('replace_existing', True)
task.set_editor_property('save', True)
options = u.FbxImportUI()
options.set_editor_property('import_materials', False)
options.set_editor_property('import_textures', False)
options.set_editor_property('mesh_type_to_import', u.FBXImportType.FBXIT_STATIC_MESH)
options.static_mesh_import_data.set_editor_property('combine_meshes', True)
options.static_mesh_import_data.set_editor_property('auto_generate_collision', False)
options.static_mesh_import_data.set_editor_property('convert_scene', False)
options.static_mesh_import_data.set_editor_property('convert_scene_unit', False)
task.set_editor_property('options', options)
task.set_editor_property('factory', u.FbxFactory())
assets.import_asset_tasks([task])
mesh = u.load_asset(folder + '/SM_BlitzRocketGrab')
assert isinstance(mesh, u.StaticMesh)
for i, slot in enumerate(mesh.static_materials):
    mesh.set_material(i, materials[str(slot.material_slot_name)])
assert u.EditorAssetLibrary.save_loaded_asset(mesh, False)
print('BLITZ_GRAB_ASSETS_READY', mesh.get_bounds())
