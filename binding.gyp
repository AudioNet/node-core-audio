{
	'targets': [
		{
			'target_name': 'NodeCoreAudio',
			'sources': [
				'NodeCoreAudio/AudioEngine.cpp',
				'NodeCoreAudio/NodeCoreAudio.cpp',
			],
			'include_dirs': [
				'<(module_root_dir)/NodeCoreAudio/',
				'<(module_root_dir)/portaudio/'
			],
			"conditions" : [
				[
					'OS!="win"', {
						"libraries" : [
							'<(module_root_dir)/gyp/lib/libportaudio.a'
						],
						'cflags!': [ '-fno-exceptions' ],
						'cflags_cc!': [ '-fno-exceptions' ],
            			'cflags_cc': [ '-std=c++0x' ]
					}
				],
				[
					'OS=="win"', {
						"include_dirs" : [ "gyp/include" ],
						"libraries" : [
							'<(module_root_dir)/gyp/lib/portaudio_x86.lib'
						],'copies': [
				            {
				              'destination': '<(module_root_dir)/build/Debug/',
				              'files': [
				                '<(module_root_dir)/gyp/lib/portaudio_x86.dll',
				                '<(module_root_dir)/gyp/lib/portaudio_x86.lib',
				              ]
				            }
				        ]
					}
				]
			]
		}
	]
}
