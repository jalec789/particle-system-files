import os
env = Environment(ENV = os.environ)
env.Append(LIBS=["GL","GLU","glut","GLEW"])
env.Append(LIBS=["png"])
env.Append(CXXFLAGS=["-g", "-std=c++11", "-Wall", "-Werror","-I/usr/include/libpng12"])

# On the department machines, you may need to uncomment these.
# env.Append(CPPPATH=["/usr/csshare/pkgs/glew-1.13.0/include"])
# env.Append(LIBPATH=["/usr/csshare/pkgs/glew-1.13.0/lib64/"])
# env.Append(RPATH=["/usr/csshare/pkgs/glew-1.13.0/lib64/"])
env.Program("particles",["main.cpp","application.cpp","gl_texture.cpp","timer.cpp",
                       "obj.cpp","gl_viewer.cpp","gl_arcball.cpp"])
