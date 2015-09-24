{
  'targets': [
    {
      'target_name': 'weave',
      'type': 'executable',
      'cflags': ['-pthread'],
      'sources': [
        'avahi_client.cc',
        'bluez_client.cc',
        'curl_http_client.cc',
        'event_http_server.cc',
        'event_task_runner.cc',
        'file_config_store.cc',
        'main.cc',
        'network_manager.cc',
        'ssl_stream.cc',
      ],
      'dependencies': [
        '../../libweave_standalone.gyp:libweave',
      ],
      'libraries': [
        '-levent',
        '-lcrypto',
        '-lexpat',
        '-lcurl',
        '-lpthread',
        '-lssl',
        '-lavahi-common',
        '-lavahi-client',
        '-levent_openssl',
      ]
    }
  ]
}