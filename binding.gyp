{
	'targets': [
		{
			'target_name': 'NodeCoreAudio',
			'sources': [
				'NodeCoreAudio/AudioEngine.cpp',
				'NodeCoreAudio/NodeCoreAudio.cpp',
			],
			'include_dirs': [
                "<!(node -e \"require('nan')\")",
				'<(module_root_dir)/NodeCoreAudio/',
				'<(module_root_dir)/portaudio/'
			],
			"conditions" : [
				[
					'OS=="linux"', {
						"libraries" : [
							'<(module_root_dir)/gyp/lib/libportaudio.a'
						],
						'cflags!': [ '-fno-exceptions' ],
						'cflags_cc!': [ '-fno-exceptions' ],
						'cflags_cc': [ '-std=c++0x' ]
					}
				],
				[
					'OS=="mac"', {
						"libraries" : [
							'<(module_root_dir)/gyp/lib/libportaudio.a',
							'/Library/Frameworks/CoreAudio.framework',
							'/Library/Frameworks/AudioToolbox.framework',
							'/Library/Frameworks/AudioUnit.framework',
							'/Library/Frameworks/CoreServices.framework',
							'/Library/Frameworks/Carbon.framework'
						],
						'cflags!': [ '-fno-exceptions' ],
						'cflags_cc!': [ '-fno-exceptions' ],
						'cflags_cc': [ '-std=c++0x' ]
					}
				],
				[
					'OS=="win"', {
						"conditions": [
							[
								'target_arch=="ia32"', {
									"libraries" : [
										'<(module_root_dir)/gyp/lib/win32/portaudio_x86.lib'
									],
									'copies': [
										{
										    'destination': '<(module_root_dir)/build/Release/',
										    'files': [
										 		'<(module_root_dir)/gyp/lib/win32/portaudio_x86.dll',
										 		'<(module_root_dir)/gyp/lib/win32/portaudio_x86.lib',
										    ]
										}
									]
								}
							],
							[
								'target_arch=="x64"', {
									"libraries" : [
										'<(module_root_dir)/gyp/lib/win64/portaudio_x64.lib'
									],
									'copies': [
										{
										    'destination': '<(module_root_dir)/build/Release/',
										    'files': [
										 		'<(module_root_dir)/gyp/lib/win64/portaudio_x64.dll',
										 		'<(module_root_dir)/gyp/lib/win64/portaudio_x64.lib',
										    ]
										}
									]
								}
							]
						],

						"include_dirs" : [ "gyp/include" ]
					}
				]
			]
		}
	]
}
