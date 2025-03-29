{
  "targets": [
    {
      "target_name": "lpx_addon",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [
        "node_addon/lpx_addon.cpp",
        "src/lpx_image.cpp"
      ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "include",
        "<!@(pkg-config --cflags opencv4 || pkg-config --cflags opencv)"
      ],
      "libraries": [
        "<!@(pkg-config --libs opencv4 || pkg-config --libs opencv)"
      ],
      "defines": [ "NAPI_DISABLE_CPP_EXCEPTIONS" ],
      "conditions": [
        ["OS==\"mac\"", {
          "xcode_settings": {
            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
            "CLANG_CXX_LIBRARY": "libc++",
            "MACOSX_DEPLOYMENT_TARGET": "10.15"
          }
        }],
        ["OS==\"linux\"", {
          "cflags": [
            "-std=c++14"
          ],
          "cflags_cc": [
            "-std=c++14"
          ]
        }]
      ]
    }
  ]
}
