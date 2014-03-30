# Copyright 2010-2014, Google Inc.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the name of Google Inc. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# IMPORTANT:
# Please don't directly include this file since the gypi files is
# automatically included.
# You may find cool techniques in the following *.gypi file.
# http://src.chromium.org/viewvc/chrome/trunk/src/build/common.gypi
{
  'variables': {

    # Top directory of third party libraries.
    'third_party_dir': '<(DEPTH)/third_party',

    # Top directory of additional third party libraries.
    'additional_third_party_dir': '<(DEPTH)/third_party',

    # Set this to true when building with Clang.
    'clang%': 0,
    # GYP has built-in assertion against 'make_global_settings' to enforce
    # that all the 'make_global_settings' must be literally equivalent.
    # In order to conform this assertion, we cannot use <(DEPTH) here because
    # <(DEPTH) will be expanded to a relative path for each gyp file.
    'clang_bin_dir%': '<(abs_depth)/third_party/llvm-build/Release+Asserts/bin',

    # Versioning stuff for Mac.
    'mac_sdk%': '10.8',
    'mac_deployment_target%': '10.5',

    # 'conditions' is put inside of 'variables' so that we can use
    # another 'conditions' in this gyp element level later. Note that
    # you can have only one 'conditions' in a gyp element.
    'variables': {
      'target_compiler': '',
      'extra_warning_cflags': '',
      'conditions': [
        ['warn_as_error!=0', {
          'extra_warning_cflags': '-Werror',
        }],
        ['OS=="win"', {
          # Variable 'MSVS_VERSION' is available only on Windows.
          'target_compiler': 'msvs<(MSVS_VERSION)',
        }],
      ],
    },

    # The target compiler such as 'msvs2010' or 'msvs2012'.
    # This value is currently used only on Windows.
    'target_compiler': '<(target_compiler)',

    # warning_cflags will be shared with Mac and Linux.
    'warning_cflags': [
      '-Wall',
      '-Wno-char-subscripts',
      '-Wno-sign-compare',
      '-Wno-deprecated-declarations',
      '-Wwrite-strings',
      '<@(extra_warning_cflags)',
    ],

    # gcc_cflags will be shared with Mac and Linux
    'gcc_cflags': [
      '-fmessage-length=0',
      '-fno-strict-aliasing',
      '-funsigned-char',
      '-include base/namespace.h',
      '-pipe',
      '-pthread',
    ],
    # linux_cflags will be used in Linux except for NaCl.
    'linux_cflags': [
      '<@(gcc_cflags)',
      '-fno-omit-frame-pointer',
      '-fstack-protector',
      '--param=ssp-buffer-size=4',
    ],
    # nacl_cflags will be used for NaCl.
    # -fno-omit-frame-pointer flag does not work correctly.
    #   http://code.google.com/p/chromium/issues/detail?id=122623
    'nacl_cflags': [
      '<@(gcc_cflags)',
    ],
    # mac_cflags will be used in Mac.
    # Xcode 4.5 which we are currently using does not support ssp-buffer-size.
    # TODO(horo): When we can use Xcode 4.6 which supports ssp-buffer-size,
    # set ssp-buffer-size in Mac.
    'mac_cflags': [
      '<@(gcc_cflags)',
      '-fno-omit-frame-pointer',
      '-fstack-protector',
    ],
    # Libraries for GNU/Linux environment.
    'linux_ldflags': [
      '-pthread',
    ],

    # Extra defines
    'additional_defines%': [],

    # Extra headers and libraries for Visual C++.
    'msvs_includes%': [],
    'msvs_libs_x86%': [],
    'msvs_libs_x64%': [],

    # enable_unittest represents if gtest-based unittest is available or not.
    'enable_unittest%': '1',

    'conditions': [
      ['OS=="mac"', {
        'clang': 1,
        'clang_bin_dir': '/Applications/Xcode.app/Contents/Developer/Toolchains'
                         '/XcodeDefault.xctoolchain/usr/bin/',
      }],
      ['target_platform=="Linux"', {
        # enable_gtk_renderer represents if mozc_renderer is supported on Linux
        # or not.
        'enable_gtk_renderer%': 1,
      }, {  # else
        'enable_gtk_renderer%': 0,
      }],
    ],
    'msvc_disabled_warnings': [
      # 'expression' : signed/unsigned mismatch
      # http://msdn.microsoft.com/en-us/library/y92ktdf2.aspx
      '4018',
      # 'argument' : conversion from 'type1' to 'type2', possible loss
      # of data
      # http://msdn.microsoft.com/en-us/library/2d7604yb.aspx
      '4244',
      # 'var' : conversion from 'size_t' to 'type', possible loss of
      # data
      # http://msdn.microsoft.com/en-us/library/6kck0s93.aspx
      '4267',
      # 'identifier' : truncation from 'type1' to 'type2'
      # http://msdn.microsoft.com/en-us/library/0as1ke3f.aspx
      '4305',
      # previous versions of the compiler did not override when
      # parameters only differed by const/volatile qualifiers.
      # http://msdn.microsoft.com/en-us/library/bb384874.aspx
      # See also the following discussion.
      # http://code.google.com/p/googlemock/issues/detail?id=141
      '4373',
      # 'type' : forcing value to bool 'true' or 'false'
      # (performance warning)
      # http://msdn.microsoft.com/en-us/library/b6801kcy.aspx
      '4800',
      # The file contains a character that cannot be represented in the
      # current code page (number). Save the file in Unicode format to
      # prevent data loss.
      # http://msdn.microsoft.com/en-us/library/ms173715.aspx
      '4819',
      # Suppress warning against functions marked as 'deprecated'.
      # http://msdn.microsoft.com/en-us/library/vstudio/8wsycdzs.aspx
      '4995',
      # Suppress warning against functions marked as 'deprecated'.
      # http://msdn.microsoft.com/en-us/library/vstudio/ttcz0bys.aspx
      '4996',
    ],

    # We wanted to have this directory under the build output directory
    # (ex. 'out' for Linux), but there is no variable defined for the top
    # level source directory, hence we create the directory in the top
    # level source directory.
    'mozc_build_tools_dir': '<(abs_depth)/mozc_build_tools/<(OS)',
    'proto_out_dir': '<(SHARED_INTERMEDIATE_DIR)/proto_out',
    'branding%': 'Mozc',
    # use_qt is 'YES' only if you want to use GUI binaries.
    'use_qt%': 'YES',

    # server_dir represents the directory where mozc_server is
    # installed. This option is only for Linux.
    'server_dir%': '/usr/lib/mozc',

    # Represents the directory where the source code of protobuf is
    # extracted. This value is ignored when 'use_libprotobuf' is 1.
    'protobuf_root': '<(additional_third_party_dir)/protobuf',

    # use_libprotobuf represents if protobuf library is used or not.
    # This option is only for Linux.
    # You should not set this flag if you want to use "dlopen" to
    # load Mozc's modules. See
    # - http://code.google.com/p/mozc/issues/detail?id=14
    # - http://code.google.com/p/protobuf/issues/detail?id=128
    # - http://code.google.com/p/protobuf/issues/detail?id=370
    # for the background information.
    'use_libprotobuf%': 0,

    # use_libzinnia represents if zinnia library is used or not.
    # This option is only for Linux.
    'use_libzinnia%': 1,

    # use_libxml represents if libxml library is used or not.
    # This option is only for Linux.
    'use_libxml%': 1,

    # use_libibus represents if ibus library is used or not.
    # This option is only for Linux.
    'use_libibus%': 0,

    # a flag whether the current build is dev-channel or not.
    'channel_dev%': '0',

    # enable_cloud_handwriting represents if cloud handwriting feature is
    # enabled or not.
    'enable_cloud_handwriting%': 0,

    # enable_http_client represents if http client feature is enabled or not.
    'enable_http_client%': 0,

    # enable ambiguous search (a.k.a. KATSUKOU-conversion).
    'enable_ambiguous_search%': 0,
    # enable typing correction.
    'enable_typing_correction%': 0,

    # The pkg-config command to get the cflags/ldflags for Linux
    # builds.  We make it customizable to allow building in a special
    # environment such like cross-platform build.
    'pkg_config_command%': 'pkg-config',

    'mozc_data_dir': '<(SHARED_INTERMEDIATE_DIR)/',
  },
  'target_defaults': {
    'variables': {
      # See http://gcc.gnu.org/onlinedocs/gcc-4.4.2/gcc/Optimize-Options.html
      'mac_release_optimization%': '2',  # Use -O2 unless overridden
      'mac_debug_optimization%': '0',    # Use -O0 unless overridden
      # See http://msdn.microsoft.com/en-us/library/aa652360(VS.71).aspx
      'win_optimization_debug%': '0',    # 0 = /Od
      'win_optimization_release%': '2',  # 2 = /Og /Oi /Ot /Oy /Ob2 /Gs /GF /Gy
      'win_optimization_custom%': '4',   # 4 = None but prevents vcbuild from
                                         # inheriting default optimization.
      # See http://msdn.microsoft.com/en-us/library/aa652367(VS.71).aspx
      'win_release_static_crt%': '0',   # 0 = /MT (nondebug static)
      'win_debug_static_crt%': '1',     # 1 = /MTd (debug static)
      'win_release_dynamic_crt%': '2',  # 2 = /MD (nondebug dynamic)
      'win_debug_dynamic_crt%': '3',    # 3 = /MDd (debug dynamic)
      # See http://msdn.microsoft.com/en-us/library/aa652352(VS.71).aspx
      'win_target_machine_x86%': '1',
      'win_target_machine_x64%': '17',
      # See http://msdn.microsoft.com/en-us/library/aa652256(VS.71).aspx
      'win_char_set_not_set%': '0',
      'win_char_set_unicode%': '1',
      'win_char_set_mbcs%': '2',
      # Extra cflags for gcc
      'release_extra_cflags%': ['-O2'],
      'debug_extra_cflags%': ['-O0', '-g'],
    },
    'configurations': {
      'Common_Base': {
        'abstract': 1,
        'msvs_configuration_attributes': {
          'CharacterSet': '<(win_char_set_unicode)',
        },
        'conditions': [
          ['branding=="GoogleJapaneseInput"', {
            'defines': ['GOOGLE_JAPANESE_INPUT_BUILD'],
          }, {
            'defines': ['MOZC_BUILD'],
          }],
          ['channel_dev==1', {
            'defines': ['CHANNEL_DEV'],
          }],
          ['OS=="linux"', {
            'ldflags': [
              '<@(linux_ldflags)',
            ],
          }],
          ['use_separate_collocation_data==1', {
            'defines': ['MOZC_USE_SEPARATE_COLLOCATION_DATA'],
          }],
          ['use_separate_connection_data==1', {
            'defines': ['MOZC_USE_SEPARATE_CONNECTION_DATA'],
          }],
          ['use_separate_dictionary==1', {
            'defines': ['MOZC_USE_SEPARATE_DICTIONARY'],
          }],
          ['use_packed_dictionary==1', {
            'defines': ['MOZC_USE_PACKED_DICTIONARY'],
          }],
          ['enable_cloud_handwriting==1', {
            'defines': ['ENABLE_CLOUD_HANDWRITING'],
          }],
          ['enable_http_client==1', {
            # TODO(peria): Considers of moving the definition and control of
            # enable_http_client and MOZC_ENABLE_HTTP_CLIENT to net/net.gyp.
            'defines': ['MOZC_ENABLE_HTTP_CLIENT'],
          }],
          ['enable_gtk_renderer==1', {
            'defines': ['ENABLE_GTK_RENDERER'],
          }],
          ['enable_unittest==1', {
            'defines': ['MOZC_ENABLE_UNITTEST'],
          }],
          ['target_platform=="Android"', {
            'defines': ['NO_USAGE_REWRITER'],
          }],
          ['enable_mode_indicator==1', {
            'defines': ['MOZC_ENABLE_MODE_INDICATOR'],
          }],
        ],
      },
      'x86_Base': {
        'abstract': 1,
        'msvs_settings': {
          'VCCLCompilerTool': {
            'conditions': [
              ['target_compiler!="msvs2010"', {
                # Windows 7 and prior still support CPUs that lack of SSE/SSE2.
                # So we explicitly disable them. We can change this setting to
                # /arch:SSE2 once Windows 7 is unsupported in Mozc.
                # Note that Visual C++ 2010 does not have /arch:IA32 and
                # does not use these enhanced instruction set by default.
                'AdditionalOptions': '/arch:IA32',
              }],
            ],
          },
          'VCLibrarianTool': {
            'AdditionalLibraryDirectories': [
              '<@(msvs_libs_x86)',
            ],
            'AdditionalLibraryDirectories!': [
              '<@(msvs_libs_x64)',
            ],
          },
          'VCLinkerTool': {
            'TargetMachine': '<(win_target_machine_x86)',
            'AdditionalOptions': [
              '/SAFESEH',
            ],
            'AdditionalLibraryDirectories': [
              '<@(msvs_libs_x86)',
            ],
            'AdditionalLibraryDirectories!': [
              '<@(msvs_libs_x64)',
            ],
            'EnableUAC': 'true',
            'UACExecutionLevel': '0',  # level="asInvoker"
            'UACUIAccess': 'false',    # uiAccess="false"
          },
        },
        'msvs_configuration_attributes': {
          'OutputDirectory': '<(build_base)/$(ConfigurationName)',
          'IntermediateDirectory': '<(build_base)/$(ConfigurationName)/obj/$(ProjectName)',
        },
        'msvs_configuration_platform': 'Win32',
      },
      'x64_Base': {
        'abstract': 1,
        'msvs_configuration_attributes': {
          'OutputDirectory': '<(build_base)/$(ConfigurationName)_x64',
          'IntermediateDirectory': '<(build_base)/$(ConfigurationName)_x64/obj/$(ProjectName)',
        },
        'msvs_configuration_platform': 'x64',
        'msvs_settings': {
          'VCLibrarianTool': {
            'AdditionalLibraryDirectories': [
              '<@(msvs_libs_x64)',
            ],
            'AdditionalLibraryDirectories!': [
              '<@(msvs_libs_x86)',
            ],
          },
          'VCLinkerTool': {
            'TargetMachine': '<(win_target_machine_x64)',
            'AdditionalLibraryDirectories': [
              '<@(msvs_libs_x64)',
            ],
            'AdditionalLibraryDirectories!': [
              '<@(msvs_libs_x86)',
            ],
          },
        },
      },
      'Win_Static_Debug_CRT_Base': {
        'abstract': 1,
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '<(win_debug_static_crt)',
          },
        },
      },
      'Win_Static_Optimize_CRT_Base': {
        'abstract': 1,
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '<(win_release_static_crt)',
          },
        },
      },
      'Win_Dynamic_Debug_CRT_Base': {
        'abstract': 1,
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '<(win_debug_dynamic_crt)',
          },
        },
      },
      'Win_Dynamic_Optimize_CRT_Base': {
        'abstract': 1,
        'msvs_settings': {
          'VCCLCompilerTool': {
            'RuntimeLibrary': '<(win_release_dynamic_crt)',
          },
        },
      },
      'Debug_Base': {
        'abstract': 1,
        'defines': [
          'DEBUG',
        ],
        'xcode_settings': {
          'COPY_PHASE_STRIP': 'NO',
          'GCC_OPTIMIZATION_LEVEL': '<(mac_debug_optimization)',
          'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES',
          'OTHER_CFLAGS': [ '<@(debug_extra_cflags)', ],
        },
        'msvs_settings': {
          'VCCLCompilerTool': {
            'Optimization': '<(win_optimization_debug)',
            'PreprocessorDefinitions': ['_DEBUG'],
            'BasicRuntimeChecks': '3',
          },
          'VCResourceCompilerTool': {
            'PreprocessorDefinitions': ['_DEBUG'],
          },
        },
        'conditions': [
          ['OS=="linux"', {
            'cflags': [
              '<@(debug_extra_cflags)',
            ],
          }],
        ],
      },
      'Optimize_Base': {
        'abstract': 1,
        'defines': [
          'NDEBUG',
          'QT_NO_DEBUG',
        ],
        'xcode_settings': {
          'DEAD_CODE_STRIPPING': 'YES',  # -Wl,-dead_strip
          'GCC_OPTIMIZATION_LEVEL': '<(mac_release_optimization)',
          'OTHER_CFLAGS': [ '<@(release_extra_cflags)', ],
        },
        'msvs_settings': {
          'VCCLCompilerTool': {
            # '<(win_optimization_release)' (that is /O2) contains /Oy option,
            # which makes debugging extremely difficult. (See b/1852473)
            # http://msdn.microsoft.com/en-us/library/8f8h5cxt.aspx
            # We can still disable FPO by using /Oy- option but the document
            # says there is an order dependency, that is, the last /Oy  or /Oy-
            # is valid.  See the following document for details.
            # http://msdn.microsoft.com/en-us/library/2kxx5t2c.aspx
            # As far as we observed, /Oy- adding in 'AdditionalOptions' always
            # appears at the end of options so using
            # '<(win_optimization_release) here is considered to be harmless.
            # Another reason to use /O2 here is b/2822535, where we observed
            # warning C4748 when we build mozc_tool with Qt libraries, which
            # are built with /O2.  We use the same optimization option between
            # Mozc and Qt just in case warning C4748 is true.
            'Optimization': '<(win_optimization_release)',
          },
        },
        'conditions': [
          ['OS=="linux"', {
            'cflags': [
              '<@(release_extra_cflags)',
            ],
          }],
        ],
      },
      'Release_Base': {
        'abstract': 1,
        'defines': [
          'NO_LOGGING',
          'IGNORE_HELP_FLAG',
          'IGNORE_INVALID_FLAG'
        ],
        'msvs_settings': {
          'VCCLCompilerTool': {
            'WholeProgramOptimization': 'true',
          },
          'VCLibrarianTool': {
            'LinkTimeCodeGeneration': 'true',
          },
          'VCLinkerTool': {
            # 1 = 'LinkTimeCodeGenerationOptionUse'
            'LinkTimeCodeGeneration': '1',
            # /PDBALTPATH is documented in Visual C++ 2010
            # http://msdn.microsoft.com/en-us/library/dd998269(VS.100).aspx
            'AdditionalOptions': ['/PDBALTPATH:%_PDB%'],
          },
        },
      },
      'Android_Base': {
        'abstract': 1,
        'defines': [
          'OS_ANDROID',
          'MOZC_ANDROID_APPLICATION_ID="<(android_application_id)"',
        ],
        'ldflags!': [  # Remove all libraries for GNU/Linux.
          '<@(linux_ldflags)',
        ],
        'ldflags': [
          '-llog',
        ],
        'conditions': [
          ['android_arch=="arm"', {
            'cflags': [
              '-mthumb',  # Force thumb interaction set for smaller file size.
            ],
          }],
        ],
      },
      #
      # Concrete configurations
      #
      'Debug': {
        'inherit_from': ['Common_Base', 'x86_Base', 'Debug_Base', 'Win_Static_Debug_CRT_Base'],
      },
      'Optimize': {
        'inherit_from': ['Common_Base', 'x86_Base', 'Optimize_Base', 'Win_Static_Optimize_CRT_Base'],
      },
      'Release': {
        'inherit_from': ['Optimize', 'Release_Base'],
      },
      'conditions': [
        ['OS=="win"', {
          'DebugDynamic': {
            'inherit_from': ['Common_Base', 'x86_Base', 'Debug_Base', 'Win_Dynamic_Debug_CRT_Base'],
          },
          'OptimizeDynamic': {
            'inherit_from': ['Common_Base', 'x86_Base', 'Optimize_Base', 'Win_Dynamic_Optimize_CRT_Base'],
          },
          'ReleaseDynamic': {
            'inherit_from': ['OptimizeDynamic', 'Release_Base'],
          },
          'Debug_x64': {
            'inherit_from': ['Common_Base', 'x64_Base', 'Debug_Base', 'Win_Static_Debug_CRT_Base'],
          },
          'Optimize_x64': {
            'inherit_from': ['Common_Base', 'x64_Base', 'Optimize_Base', 'Win_Static_Optimize_CRT_Base'],
          },
          'Release_x64': {
            'inherit_from': ['Optimize_x64', 'Release_Base'],
          },
        }],
        ['target_platform=="Android"', {
          'Debug_Android': {
            'inherit_from': ['Common_Base', 'Android_Base', 'Debug_Base'],
            # We won't debug target's .so file so remove debug symbol.
            # If the symbol is required, remove following line.
            'cflags!': ['-g'],
          },
          'Release_Android': {
            'inherit_from': ['Common_Base', 'Android_Base', 'Optimize_Base', 'Release_Base'],
          },
        }],
      ],
    },
    'default_configuration': 'Debug',
    'defines': [
      '<@(additional_defines)',
    ],
    'include_dirs': [
      '<(DEPTH)',
      '<(SHARED_INTERMEDIATE_DIR)',
    ],
    'conditions': [
      ['OS=="win"', {
        'defines': [
          'COMPILER_MSVC',
          'BUILD_MOZC',  # for ime_shared library
          'ID_TRACE_LEVEL=1',
          'OS_WIN',
          'UNICODE',
          'WIN32',
          'WIN32_IE=0x0600',
          'WINVER=0x0501',
          'WIN32_LEAN_AND_MEAN',
          '_ATL_ALL_WARNINGS',
          '_ATL_ALLOW_CHAR_UNSIGNED',
          '_ATL_CSTRING_EXPLICIT_CONSTRUCTORS',
          '_CRT_SECURE_NO_DEPRECATE',
          '_MIDL_USE_GUIDDEF_',
          '_STL_MSVC',
          '_UNICODE',
          '_WIN32',
          '_WIN32_WINDOWS=0x0501',
          '_WIN32_WINNT=0x0501',
          '_WINDOWS',
        ],
        'include_dirs': [
          '<@(msvs_includes)',
          '<(additional_third_party_dir)/wtl/files/include',
          # Add atl_wrapper dir into the 'include_dirs' so that we can
          # include the header file as <atlbase_mozc.h>, which
          # is more lintian-friendly than "atlbase_mozc.h".
          # See b/5101916 for the background information.
          '<(DEPTH)/win32/atl_wrapper',
        ],
        'msvs_cygwin_shell': 0,
        'msvs_disabled_warnings': ['<@(msvc_disabled_warnings)'],  # /wdXXXX
        'msvs_settings': {
          'VCCLCompilerTool': {
            'BufferSecurityCheck': 'true',         # /GS
            'DebugInformationFormat': '3',         # /Zi
            'DefaultCharIsUnsigned': 'true',       # /J
            'EnableFunctionLevelLinking': 'true',  # /Gy
            'EnableIntrinsicFunctions': 'true',    # /Oi
            'ExceptionHandling': '2',              # /EHs
            'ForcedIncludeFiles': ['base/namespace.h'],
                                                   # /FI<header_file.h>
            'SuppressStartupBanner': 'true',       # /nologo
            'TreatWChar_tAsBuiltInType': 'false',  # /Zc:wchar_t-
            'WarningLevel': '3',                   # /W3
            'OmitFramePointers': 'false',          # /Oy-
          },
          'VCLinkerTool': {
            'AdditionalDependencies': [
              'advapi32.lib',
              'comdlg32.lib',
              'delayimp.lib',
              'gdi32.lib',
              'imm32.lib',
              'kernel32.lib',
              'ole32.lib',
              'oleaut32.lib',
              'psapi.lib',
              'shell32.lib',
              'user32.lib',
              'uuid.lib',
            ],
            'DataExecutionPrevention': '2',        # /NXCOMPAT
            'EnableCOMDATFolding': '2',            # /OPT:ICF
            'GenerateDebugInformation': 'true',    # /DEBUG
            'LinkIncremental': '1',                # /INCREMENTAL:NO
            'OptimizeReferences': '2',             # /OPT:REF
            'RandomizedBaseAddress': '2',          # /DYNAMICBASE
            'target_conditions': [
              # /TSAWARE is valid only on executable target.
              ['_type=="executable"', {
                'TerminalServerAware': '2',        # /TSAWARE
              }],
            ],
          },
          'VCResourceCompilerTool': {
            'PreprocessorDefinitions': [
              'MOZC_RES_USE_TEMPLATE=1',
            ],
            'AdditionalIncludeDirectories': [
              '<(SHARED_INTERMEDIATE_DIR)/',
              '<(DEPTH)/',
            ],
          },
        },
      }],
      ['OS=="linux"', {
        'defines': [
          'OS_LINUX',
        ],
        'cflags': [
          '<@(warning_cflags)',
          '-fPIC',
          '-fno-exceptions',
        ],
        'cflags_cc': [
          # We use deprecated <hash_map> and <hash_set> instead of upcoming
          # <unordered_map> and <unordered_set>.
          '-Wno-deprecated',
        ],
        'conditions': [
          ['clang==1', {
            'cflags': [
              '-Wtype-limits',
            ],
            'cflags_cc': [
              '-Wno-covered-switch-default',
              '-Wno-unnamed-type-template-args',
              '-Wno-c++11-narrowing',
              '-std=gnu++0x',
            ],
          }],
          ['clang==0 and target_platform!="Windows"', {
            'cflags_cc': [
              '-std=gnu++0x',
            ],
          }],
          ['target_platform!="NaCl"', {
            'cflags': [
              '<@(linux_cflags)',
            ],
          }],
          ['target_platform=="NaCl"', {
            'target_conditions' : [
              ['_toolset=="host"', {
                'cflags': [
                  '<@(linux_cflags)',
                ],
              }],
              ['_toolset=="target"', {
                'cflags': [
                  '<@(nacl_cflags)',
                ],
                'cflags_cc': [
                  '-std=gnu++0x',
                ],
                'ldflags!': [  # Remove all libraries for GNU/Linux.
                  '<@(linux_ldflags)',
                ],
                'defines': [
                  'MOZC_USE_PEPPER_FILE_IO',
                ],
                'include_dirs': [
                  '<(nacl_sdk_root)/include',
                ],
              }],
              ['_toolset=="target" and _type=="static_library"', {
                # PNaCl's artools.py doesn't support thin archive file.
                # (crbug/165096)
                # But GYP creates thin archive files for static_library.
                # To avoid this issue we turn on this flag.
                'standalone_static_library': 1,
              }],
            ]
          }],
        ],
      }],
      ['OS=="mac"', {
        'defines': [
          'OS_MACOSX',
        ],
        'mac_framework_dirs': [
          '<(mac_dir)/Releases/GoogleBreakpad',
          '<(DEPTH)/mozc_build_tools/mac',
        ],
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'NO',  # -fno-exceptions
          'GCC_SYMBOLS_PRIVATE_EXTERN': 'NO',  # No -fvisibility=hidden
          'OTHER_CFLAGS': [
            '<@(mac_cflags)',
          ],
          'WARNING_CFLAGS': ['<@(warning_cflags)'],
          'MACOSX_DEPLOYMENT_TARGET': '<(mac_deployment_target)',
          'SDKROOT': 'macosx<(mac_sdk)',
          'PYTHONPATH': '<(abs_depth)/',
          'conditions': [
            ['clang==1', {
              'CC': '<(clang_bin_dir)/clang',
              'LDPLUSPLUS': '<(clang_bin_dir)/clang++',
              'CLANG_WARN_CXX0X_EXTENSIONS': 'NO',
              'GCC_VERSION': 'com.apple.compilers.llvm.clang.1_0',
              'WARNING_CFLAGS': [
                '-Wno-c++11-narrowing',
                '-Wno-covered-switch-default',
                '-Wno-unnamed-type-template-args',
              ],
              'OTHER_CPLUSPLUSFLAGS': [
                '$(inherited)', '-std=gnu++11',
              ],
            }],
          ],
        },
        'link_settings': {
          'libraries': [
            '$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
            '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
            '$(SDKROOT)/System/Library/Frameworks/IOKit.framework',
            '$(SDKROOT)/System/Library/Frameworks/Security.framework',
            '$(SDKROOT)/System/Library/Frameworks/SystemConfiguration.framework',
          ],
        },
      }],
    ],
  },
  'conditions': [
    ['target_platform=="Linux" and clang==1', {
      'make_global_settings': [
        ['CC', '<(clang_bin_dir)/clang'],
        ['CXX', '<(clang_bin_dir)/clang++'],
        ['LINK', '$(CXX)'],
        ['CC.host', '$(CC)'],
        ['CXX.host', '$(CXX)'],
        ['LINK.host', '$(LINK)'],
      ],
    }],
    ['target_platform=="NaCl"', {
      'variables': {
        'pnacl_bin_dir%':
            '<(nacl_sdk_root)/toolchain/linux_x86_pnacl/newlib/bin',
      },
      'make_global_settings': [
        ['AR.target', '<(pnacl_bin_dir)/pnacl-ar'],
        ['AS.target', '<(pnacl_bin_dir)/pnacl-as'],
        ['CC.target', '<(pnacl_bin_dir)/pnacl-clang'],
        ['CXX.target', '<(pnacl_bin_dir)/pnacl-clang++'],
        ['LD.target', '<(pnacl_bin_dir)/pnacl-ld'],
        ['LINK.target', '<(pnacl_bin_dir)/pnacl-clang++'],
        ['NM.target', '<(pnacl_bin_dir)/pnacl-nm'],
        ['RANLIB.target', '<(pnacl_bin_dir)/pnacl-ranlib'],
        ['STRIP.target', '<(pnacl_bin_dir)/pnacl-strip'],
      ],
    }],
    ['target_platform=="Android"', {
      'variables': {
        'ndk_bin_dir%':
            '<(mozc_build_tools_dir)/ndk-standalone-toolchain/<(android_arch)/bin',
      },
      'conditions': [
        ['android_arch=="arm"', {
          'variables': {
            'toolchain_prefix': 'arm-linux-androideabi',
          },
        }],
        ['android_arch=="x86"', {
          'variables': {
            'toolchain_prefix': 'i686-linux-android',
          },
        }],
        ['android_arch=="mips"', {
          'variables': {
            'toolchain_prefix': 'mipsel-linux-android',
          },
        }],
      ],
      'make_global_settings': [
        ['AR.target', '<(ndk_bin_dir)/<(toolchain_prefix)-ar'],
        ['AS.target', '<(ndk_bin_dir)/<(toolchain_prefix)-as'],
        ['CC.target', '<(ndk_bin_dir)/<(toolchain_prefix)-gcc'],
        ['CXX.target', '<(ndk_bin_dir)/<(toolchain_prefix)-g++'],
        ['LD.target', '<(ndk_bin_dir)/<(toolchain_prefix)-ld'],
        ['LINK.target', '<(ndk_bin_dir)/<(toolchain_prefix)-g++'],
        ['NM.target', '<(ndk_bin_dir)/<(toolchain_prefix)-nm'],
        ['RANLIB.target', '<(ndk_bin_dir)/<(toolchain_prefix)-ranlib'],
        ['STRIP.target', '<(ndk_bin_dir)/<(toolchain_prefix)-strip'],
      ],
    }],
  ],
  'xcode_settings': {
    'SYMROOT': '<(build_base)',
    'GCC_INLINES_ARE_PRIVATE_EXTERN': 'YES',
  },
}
