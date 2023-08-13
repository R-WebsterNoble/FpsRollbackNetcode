#pragma once

#include "PlayerInputsSynchronizer.h"
#include "lib/readerwritercircularbuffer.h"
#include "Rollback/GameState.h"

struct PlayerInputsSynchronizerPacket
{
	OptInt TickNum;
	int TickCount = 0;
	CPlayerInput Inputs[MAX_TICKS_TO_TRANSMIT];
};

union PlayerInputsSynchronizerPacketBytesUnion
{
	PlayerInputsSynchronizerPacket packet;
	char buff[sizeof PlayerInputsSynchronizerPacket] = { 0 };
};

class CPlayerInputsSynchronizer
{
public:
	void Enqueue(int tickNum, const CPlayerInput &playerInput);
	bool GetPaket(OUT char* buff, OUT size_t& size);
	std::pair<int, int> LoadPaket(char* buff, size_t size, RingBuffer<CPlayerInput> &playerInputsBuffer);

private:
	AtomicOptInt m_lastTickAcked;
	OptInt m_tickNum;
	RingBuffer<CPlayerInput> m_playerInputsBuffer = RingBuffer<CPlayerInput>();
};


inline std::ostream& operator<<(std::ostream& out, PlayerInputsSynchronizerPacket const& rhs)
{
    //ClientToServerUpdate{ packetTypeCode = 'r', playerNum = 1, tickCount = 1, tickNum = 1, ackServerUpdateNumber = 1, playerInputs = {CPlayerInput{Vec2{0.1, 0.1}, EInputFlag::MoveForward}} };
    /*packetTypeCode*/
    out << "PlayerInputsSynchronizerPacket{ ";
    out << "/*TickNum*/ " << rhs.TickNum << ", "
        << "/*TickCount*/ " << rhs.TickCount << ", "
        << "{ ";
    for (int i = 0; i < rhs.TickCount; ++i)
    {
        out << rhs.Inputs[i];// "CPlayerInput{Vec2{" << playerInputs[i].mouseDelta.x << ", " << playerInputs[i].mouseDelta.y << "}, " << playerInputs[i].playerActions;
        if (i < rhs.TickCount - 1)
            out << ", ";
        else
            out << " ";
    }
    out << " }";
    // precise formatting depends on your use case
    return out;
}