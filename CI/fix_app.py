#!/usr/bin/env python3

import sys,os.path,subprocess

APP_NAME = 'RPAN Studio.app'
QT_PATH = f'{APP_NAME}/Contents/PlugIns'

def run(cmd):
    try:
        return str(subprocess.check_output(cmd, shell=True), 'utf-8').strip()
    except:
        return ''

if not os.path.exists(APP_NAME):
    print(f'Please run in the same folder as {APP_NAME}')
    sys.exit(1)

if not os.path.exists(f'{APP_NAME}/Contents/Frameworks'):
    os.mkdir(f'{APP_NAME}/Contents/Frameworks')

qt_dirs = ['platforms', 'styles', 'iconengines', 'imageformats', 'printsupport']
for d in qt_dirs:
    if not os.path.exists(f'{QT_PATH}/{d}'):
        os.mkdir(f'{QT_PATH}/{d}')

# these should probably copy from QT_DIR instead of hard coding the path
run(f'cp /usr/local/opt/qt/plugins/platforms/libqcocoa.dylib "{QT_PATH}/platforms"')
run(f'cp /usr/local/opt/qt/plugins/styles/*.dylib "{QT_PATH}/styles"')
run(f'cp /usr/local/opt/qt/plugins/iconengines/*.dylib "{QT_PATH}/iconengines"')
run(f'cp /usr/local/opt/qt/plugins/imageformats/*.dylib "{QT_PATH}/imageformats"')
run(f'cp /usr/local/opt/qt/plugins/printsupport/*.dylib "{QT_PATH}/printsupport"')

conf = open(f'{APP_NAME}/Contents/Resources/qt.conf', 'w')
conf.write("""
[Paths]
Plugins = Frameworks
""")
conf.flush()
conf.close()

apps = ['obs', 'obs-ffmpeg-mux', 'obs-browser-page']
apps += [os.path.basename(i) for i in run(f'find "{APP_NAME}/Contents/Resources" -name *.dylib -or -name *.so').split('\n')]
apps += [os.path.basename(i) for i in run(f'find "{APP_NAME}/Contents/MacOS" -name *.dylib -or -name *.so').split('\n')]
apps += [os.path.basename(i) for i in run(f'find "{APP_NAME}/Contents/PlugIns" -name *.dylib -or -name *.so').split('\n')]

exec_path = f'{APP_NAME}/Contents/MacOS'

def get_deps(path):
    deps = run(f'otool -L "{path}" | grep -vE "{path}|/System|/usr/lib"')
    output = {
        'frameworks': [],
        'shared_objects': [],
        'dylibs': [],
    }
    for dep in deps.split('\n'):
        dep = dep[:dep.find('(')].strip()
        if dep.find('.framework') > 0:
            output['frameworks'].append(dep)
        elif dep.find('.so') > 0:
            output['shared_objects'].append(dep)
        elif dep.find('.dylib') > 0:
            output['dylibs'].append(dep)
    return output

def _process(parent, inp, binary, dep_path, depth = 0):
    indent = '   ' * depth

    results = []
    dest_path = f'{APP_NAME}/Contents/Frameworks/{binary}'
    new_dep = f'@rpath/Frameworks/{binary}'
    results.append((inp, new_dep, parent))
    if not os.path.exists(dest_path):
        print(f'{indent}   Copying {dep_path} to Frameworks folder')
        run(f'cp -RH "{dep_path}" "{APP_NAME}/Contents/Frameworks/"')
        run(f'chmod -R u+w "{APP_NAME}/Contents/Frameworks/{binary}"')
        print(f'{indent}   Updating self ref "{new_dep}" => "{dest_path}"')
        run(f'install_name_tool -id {new_dep} "{dest_path}"')
        print(f'{indent}   Updating RPATH')
        run(f'install_name_tool -add_rpath @executable_path/ "{dest_path}"')
        processed = process_dependencies(dest_path, depth + 1)
        print(f'{indent}   Updating dependencies of {inp}')
        for dep in processed:
            run(f'install_name_tool -change "{dep[0]}" "{dep[1]}" "{dep[2]}"')
    return results

def process_dependencies(path, depth = 0):
    indent = '   ' * depth

    if not os.path.exists(path):
        print(f'{indent}  Dependency not found: {path}\n')
        return []

    print(f'{indent} Processing: {path}')

    deps = get_deps(path)

    results = []

    for framework in deps['frameworks']:
        print(f'{indent}  Framework: {framework}')
        if framework.startswith('/'):
            ext = framework.find('.framework')
            dep_path = framework[:ext + len('.framework')]
            binary = framework[framework.rfind('/', 0, ext) + 1:]
            results += _process(path, framework, binary, dep_path, depth)
        elif framework.startswith('@executable_path'):
            ext = framework.find('.framework')
            dep_path = os.path.normpath(f'{exec_path}/{framework.replace("@executable_path", "")}')
            binary = dep_path[dep_path.rfind('/', 0, ext) + 1:]
            results += _process(path, framework, binary, dep_path, depth)

    for shared in deps['shared_objects']:
        print(f'{indent}  Shared Object: {shared}')
        if shared.startswith('/'):
            dep_path = shared
            binary = shared[shared.rfind('/') + 1:]
            results += _process(path, shared, binary, dep_path, depth)
        elif shared.startswith('@rpath'):
            binary = os.path.basename(shared)
            abs_path = run(f'find "`pwd`/{APP_NAME}" -name "{binary}"')
            results += _process(path, shared, binary, abs_path, depth)

    for dylib in deps['dylibs']:
        print(f'{indent}  Dynamic Library: {dylib}')
        if dylib.startswith('/'):
            dep_path = dylib
            binary = dylib[dylib.rfind('/') + 1:]
            results += _process(path, dylib, binary, dep_path, depth)
        elif dylib.startswith('@rpath'):
            binary = os.path.basename(dylib)
            abs_path = run(f'find "`pwd`/{APP_NAME}" -name "{binary}"')
            results += _process(path, dylib, binary, abs_path, depth)

    return results

for app in apps:
    if len(app) == 0:
        continue

    print('==================')
    print(f'Processing {app}')
    print('==================\n')
    path = run(f'find "`pwd`/{APP_NAME}" -name {app}')
    processed = process_dependencies(path)
    print(f'Updating dependencies of {app}')
    for dep in processed:
        print(f'install_name_tool -change "{dep[0]}" "{dep[1]}" "{dep[2]}"')
        run(f'install_name_tool -change "{dep[0]}" "{dep[1]}" "{dep[2]}"')
    p = os.path.dirname(path)
    depth = ''
    while not p.endswith('Contents'):
        p = p[:p.rfind('/')]
        depth += '../'
    run(f'install_name_tool -add_rpath @executable_path/{depth} "{path}"')
    print('\n')

