# 宿主编译三元组覆盖：对应 vcpkg 内置 x64-windows。
#
# 显式复述内置值（动态 CRT / 动态库）以免覆盖后行为漂移，仅额外限定只构建 Release。
# 用途：宿主依赖（如为 qttools 提供 lupdate/lrelease 所需的 host qtbase）同样只需 Release，
# 否则宿主侧仍会构建 Debug + Release 两套，构建时间翻倍。
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

# 仅构建 Release。
set(VCPKG_BUILD_TYPE release)
