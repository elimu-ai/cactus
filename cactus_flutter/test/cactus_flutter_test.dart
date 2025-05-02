import 'package:flutter_test/flutter_test.dart';
import 'package:cactus_flutter/cactus_flutter.dart';
import 'package:cactus_flutter/cactus_flutter_platform_interface.dart';
import 'package:cactus_flutter/cactus_flutter_method_channel.dart';
import 'package:plugin_platform_interface/plugin_platform_interface.dart';

class MockCactusFlutterPlatform
    with MockPlatformInterfaceMixin
    implements CactusFlutterPlatform {

  @override
  Future<String?> getPlatformVersion() => Future.value('42');
}

void main() {
  final CactusFlutterPlatform initialPlatform = CactusFlutterPlatform.instance;

  test('$MethodChannelCactusFlutter is the default instance', () {
    expect(initialPlatform, isInstanceOf<MethodChannelCactusFlutter>());
  });

  test('getPlatformVersion', () async {
    CactusFlutter cactusFlutterPlugin = CactusFlutter();
    MockCactusFlutterPlatform fakePlatform = MockCactusFlutterPlatform();
    CactusFlutterPlatform.instance = fakePlatform;

    expect(await cactusFlutterPlugin.getPlatformVersion(), '42');
  });
}
