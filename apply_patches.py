from os.path import join, isfile

Import("env")


FRAMEWORK_DIR = env.PioPlatform().get_package_dir("framework-spl-gd32")
patchflag_path = join(FRAMEWORK_DIR, ".patching-done")

# patch file only if we didn't do it before
if not isfile(join(FRAMEWORK_DIR, ".patching-done")):
    original_file = join(FRAMEWORK_DIR, "gd32", "cmsis", "variants", "gd32f10x", "system_gd32f10x.c")
    patched_file = join("patches", "Gd32F103RCT6", "system_gd32f10x_define_clock.patch")

    assert isfile(original_file) and isfile(patched_file)

    env.Execute("git apply %s %s" % (original_file, patched_file))
    # env.Execute("touch " + patchflag_path)


    def _touch(path):
        with open(path, "w") as fp:
            fp.write("")

    env.Execute(lambda *args, **kwargs: _touch(patchflag_path))