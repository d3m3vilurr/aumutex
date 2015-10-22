{
  "targets": [
      {
          "include_dirs": [ "<!(node -e \"require('nan')\")" ],
          "target_name": "aumutex",
          "cflags_cc": [ '-fexceptions' ],
          "sources": [
              "./aumutex.cc"
          ]
      }
  ]
}
