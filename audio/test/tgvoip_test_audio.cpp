//
// libtgvoip is free and unencumbered public domain software.
// For more information, see http://unlicense.org or the UNLICENSE file
// you should have received with this source code distribution.
//

#include "MockReflector.h"
#include "VoIPController.h"
#include "webrtc_dsp/common_audio/wav_file.h"

#include <array>
#include <cmath>
#include <gtest/gtest.h>
#include <openssl/rand.h>
#include <string>
#include <time.h>

using namespace tgvoip;
using namespace std;

static void sleepForTimeInterval(float seconds)
{
	timespec val;
	val.tv_sec = trunc(seconds);
	val.tv_nsec = (seconds - trunc(seconds)) * 1e9;
	nanosleep(&val, NULL);
}

class TestAudio
{
	VoIPController* controller1;
	VoIPController* controller2;
	string testWavFilePath;
	
	void initControllers();
	void destroyControllers();

public :

	TestAudio();	
	~TestAudio();

	static void testBasicOperation();
	static void testAllocationAndDeallocation();
	static void testInitTimeout();
	static void testPacketTimeout();
};

TestAudio::TestAudio()
{
	// This file must be mono 16-bit 48000hz
	testWavFilePath = "./voip_test_input.wav";
	
	initControllers();
}

TestAudio::~TestAudio()
{
	destroyControllers();
}

void TestAudio::initControllers()
{
	controller1 = new VoIPController();
	controller2 = new VoIPController();
	
	array<array<uint8_t, 16>, 2> peerTags=test::MockReflector::GeneratePeerTags();
	vector<Endpoint> endpoints1;
	IPv4Address localhost("127.0.0.1");
	IPv6Address emptyV6;
	endpoints1.push_back(Endpoint(1, 1033, localhost, emptyV6, Endpoint::Type::UDP_RELAY, peerTags[0].data()));
	controller1->SetRemoteEndpoints(endpoints1, false, 76);
	vector<Endpoint> endpoints2;
	endpoints2.push_back(Endpoint(1, 1033, localhost, emptyV6, Endpoint::Type::UDP_RELAY, peerTags[1].data()));
	controller2->SetRemoteEndpoints(endpoints2, false, 76);
	
	char encryptionKey[256];
	RAND_bytes((uint8_t*)encryptionKey, sizeof(encryptionKey));
	controller1->SetEncryptionKey(encryptionKey, true);
	controller2->SetEncryptionKey(encryptionKey, false);
}

void TestAudio::destroyControllers()
{
	controller1->Stop();
	delete controller1;
	controller2->Stop();
	delete controller2;
}

void TestAudio::testBasicOperation()
{
	webrtc::WavWriter wavWriter("output.wav", 48000, 1);
	
	test::MockReflector reflector("127.0.0.1", 1033);
	reflector.Start();

	{
		TestAudio ta;	

		webrtc::WavReader wavReader1(ta.testWavFilePath);
		webrtc::WavReader wavReader2(ta.testWavFilePath);
		
		ta.controller1->SetAudioDataCallbacks([&wavReader1](int16_t* data, size_t len){
			wavReader1.ReadSamples(len, data);
		}, [](int16_t* data, size_t len){
			
		});
		
		ta.controller2->SetAudioDataCallbacks([&wavReader2](int16_t* data, size_t len){
			wavReader2.ReadSamples(len, data);
		}, [&wavWriter](int16_t* data, size_t len){
			wavWriter.WriteSamples(data, len);
		});
		
		ta.controller1->Start();
		ta.controller2->Start();
		ta.controller1->Connect();
		ta.controller2->Connect();
		sleepForTimeInterval(10.0);
	}
	
	reflector.Stop();
}

void TestAudio::testAllocationAndDeallocation()
{
	test::MockReflector reflector("127.0.0.1", 1033);
	reflector.Start();
	
	for(int i=0;i<10;i++)
	{
		TestAudio ta;

		webrtc::WavReader wavReader(ta.testWavFilePath);
		
		ta.controller1->SetAudioDataCallbacks([&wavReader](int16_t* data, size_t len){
			wavReader.ReadSamples(len, data);
		}, [](int16_t* data, size_t len){
			
		});
		
		ta.controller2->SetAudioDataCallbacks([](int16_t* data, size_t len){
			
		}, [](int16_t* data, size_t len){
			
		});
		
		ta.controller1->Start();
		ta.controller2->Start();
		ta.controller1->Connect();
		ta.controller2->Connect();
		sleepForTimeInterval(3.0);
	}
	
	reflector.Stop();
}

void TestAudio::testInitTimeout()
{
	TestAudio ta;

	VoIPController::Config config;
	config.enableNS=config.enableAEC=config.enableAGC=false;
	config.enableCallUpgrade=false;
	config.initTimeout=3.0;
	ta.controller1->SetConfig(config);
	ta.controller1->Start();
	ta.controller1->Connect();
	sleepForTimeInterval(1.5);
	ASSERT_EQ(ta.controller1->GetConnectionState(), STATE_WAIT_INIT_ACK);
	sleepForTimeInterval(2.0);
	ASSERT_EQ(ta.controller1->GetConnectionState(), STATE_FAILED);
	ASSERT_EQ(ta.controller1->GetLastError(), ERROR_TIMEOUT);
}

void TestAudio::testPacketTimeout()
{
	test::MockReflector reflector("127.0.0.1", 1033);
	reflector.Start();

	{
		TestAudio ta;
		
		webrtc::WavReader wavReader(ta.testWavFilePath);
		ta.controller1->SetAudioDataCallbacks([&wavReader](int16_t* data, size_t len){
			wavReader.ReadSamples(len, data);
		}, [](int16_t* data, size_t len){
			
		});
		
		ta.controller2->SetAudioDataCallbacks([](int16_t* data, size_t len){
			
		}, [](int16_t* data, size_t len){
			
		});
		
		VoIPController::Config config;
		config.enableNS=config.enableAEC=config.enableAGC=false;
		config.enableCallUpgrade=false;
		config.initTimeout=3.0;
		config.recvTimeout=1.5;
		ta.controller1->SetConfig(config);
		config.recvTimeout=5.0;
		ta.controller2->SetConfig(config);
		
		ta.controller1->Start();
		ta.controller2->Start();
		ta.controller1->Connect();
		ta.controller2->Connect();
		sleepForTimeInterval(2.5);
		ASSERT_EQ(ta.controller1->GetConnectionState(), STATE_ESTABLISHED);
		ASSERT_EQ(ta.controller2->GetConnectionState(), STATE_ESTABLISHED);
		reflector.SetDropAllPackets(true);
		sleepForTimeInterval(2.5);
		ASSERT_EQ(ta.controller1->GetConnectionState(), STATE_FAILED);
		ASSERT_EQ(ta.controller1->GetLastError(), ERROR_TIMEOUT);
		ASSERT_EQ(ta.controller2->GetConnectionState(), STATE_RECONNECTING);
	}
	
	reflector.Stop();
}

TEST(TestAudio, testBasicOperation)
{
	TestAudio::testBasicOperation();
}

TEST(TestAudio, testAllocationAndDeallocation)
{
	TestAudio::testAllocationAndDeallocation();
}

TEST(TestAudio, testInitTimeout)
{
	TestAudio::testInitTimeout();
}

TEST(TestAudio, testPacketTimeout)
{
	TestAudio::testPacketTimeout();
}

int main(int argc, char* argv[])
{
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

