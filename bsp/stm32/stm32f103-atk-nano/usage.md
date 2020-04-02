# 使用说明

使用rt-thread的env工具在该目录下执行以下命令：

```
pkgs --upgrade ## 确保env工具是最新版
pkgs --update
```

> u8g2 如果编译失败，使用abf1623323e09c24fc8c8fec8704fb079a1dfb45

> infrared 使用 https://github.com/z14git/infrared_framework/tree/aiotlab

> cJSON 使用 https://github.com/z14git/cJSON/tree/lab

> fal 使用 https://github.com/z14git/fal/tree/aiotlab

```
scons --target=mdk5 -s
```

打开project.uvprojx编译程序
