#pragma once

#include <flatbuffers/flatbuffer_builder.h>

#include "ClientToServerUpdate_generated.h"
#include "PlayerInputsSynchronizer.h"
#include "lib/readerwritercircularbuffer.h"
#include "Rollback/GameState.h"

struct CPlayerInputsSynchronizerPacket
{
	OptInt TickNum;
	int TickCount = 0;
	CPlayerInput Inputs[MAX_TICKS_TO_TRANSMIT];
};

union PlayerInputsSynchronizerPacketBytesUnion
{
	CPlayerInputsSynchronizerPacket packet;
	char buff[sizeof packet] = { 0 };
};

class CPlayerInputsSynchronizer
{
public:
	void Enqueue(int tickNum, const CPlayerInput &playerInput);
    void Ack(OptInt ack);
	bool GetPaket(flatbuffers::FlatBufferBuilder& builder,
	              OUT flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>&
	              synchronizer,
	              const OptInt* lastTickAcked = nullptr);
    void UpdateFromPacket(const FlatBuffPacket::PlayerInputsSynchronizer* sync);
    OptInt& GetLastTickAcked() { return m_lastTickAcked; };
	static std::tuple<int, int, std::vector<CPlayerInput>> ParsePaket(const FlatBuffPacket::PlayerInputsSynchronizer* sync, OptInt& lastTickAcked);

private:
	OptInt m_lastTickAcked;
	OptInt m_tickNum;
	RingBuffer<CPlayerInput> m_playerInputsBuffer = RingBuffer<CPlayerInput>();
};


inline std::ostream& operator<<(std::ostream& out, CPlayerInputsSynchronizerPacket const& rhs)
{
    //CClientToServerUpdate{ packetTypeCode = 'r', playerNum = 1, tickCount = 1, tickNum = 1, ackServerUpdateNumber = 1, playerInputs = {CPlayerInput{Vec2{0.1, 0.1}, EInputFlag::MoveForward}} };
    /*packetTypeCode*/
    out << "CPlayerInputsSynchronizerPacket{ ";
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
