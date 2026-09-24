# 静态链接 x64 — /MT 运行时，适合独立分发
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE static)
set(VCPKG_LIBRARY_LINKAGE static)

set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} /MP")
set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} /MP")

# 仅构建 Release：本项目不发布 Debug 产物，可省去每个依赖的 Debug 半程，构建时间约减半。
set(VCPKG_BUILD_TYPE release)
