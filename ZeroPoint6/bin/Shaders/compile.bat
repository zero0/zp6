
rem for /r %%f in (*.vert;*.frag) do glslangValidator -g0 -Os -V %%f -o %%f.spv
for /r %%f in (*.vert;*.frag) do glslangValidator -g -Od -x -V -o "%%f.spv" "%%f"
