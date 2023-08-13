#pragma once

#include "PlayerInputsSynchronizer.h"
#include "lib/readerwritercircularbuffer.h"
#include "Rollback/GameState.h"

struct PlayerInputsSynchronizerClientSendPacket
{
	OptInt TickNum;
	int TickCount = 0;
	CPlayerInput Inputs[MAX_TICKS_TO_TRANSMIT];
};

union PlayerInputsSynchronizerClientSendPacketBytesUnion
{
	PlayerInputsSynchronizerClientSendPacket packet;
	char buff[sizeof PlayerInputsSynchronizerClientSendPacket] = { 0 };
};

class CPlayerInputsSynchronizerSender
{
public:
	void Enqueue(int tickNum, CPlayerInput playerInput);
	bool GetPaket(OUT char* buff, OUT size_t& size);
	void Ack(OptInt tickNum);

private:
	OptInt m_lastTickAcked;
	OptInt m_tickNum;
	RingBuffer<CPlayerInput> m_playerInputsBuffer = RingBuffer<CPlayerInput>();
};


class CPlayerInputsSynchronizerReceiver
{
public:
	OptInt LoadPaket(char* buff, size_t size, RingBuffer<CPlayerInput>& playerInputsBuffer);
private:
	OptInt m_lastTickReceived;
};

