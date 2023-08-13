#include "PlayerInputsSynchronizer.h"



void CPlayerInputsSynchronizer::Enqueue(int tickNum, const CPlayerInput &playerInput)
{
	m_tickNum.I = tickNum;
	(*m_playerInputsBuffer.GetAt(tickNum)) = playerInput;
}

bool CPlayerInputsSynchronizer::GetPaket(OUT char* buff, OUT size_t& size)
{
	PlayerInputsSynchronizerPacketBytesUnion* packet = reinterpret_cast<PlayerInputsSynchronizerPacketBytesUnion*>(buff);

	if(!m_tickNum.has_value())
	{
		packet->packet.TickNum = OptInt{};
		packet->packet.TickCount = 0;
		size = sizeof PlayerInputsSynchronizerPacket - sizeof packet->packet.Inputs;
		return true;
	}

	const int firstTickToSend = m_lastTickAcked.has_value() ? m_lastTickAcked.value() + 1 : 0;
	const int lastTickToSend = m_tickNum.value();
	const int count = (lastTickToSend - firstTickToSend) + 1;

	if (count > MAX_TICKS_TO_TRANSMIT)
	{
		gEnv->pLog->LogToFile("Attempted Send %i inputs but max is %i", count, MAX_TICKS_TO_TRANSMIT);
		size = 0;
		return false;
	}

	packet->packet.TickNum.I = lastTickToSend;
	packet->packet.TickCount = count;	

	for (int i = 0; i < count; ++i)
	{
		packet->packet.Inputs[i] = *(m_playerInputsBuffer.GetAt(firstTickToSend+i));
	}

	size = sizeof PlayerInputsSynchronizerPacket - (sizeof packet->packet.Inputs[0] * (MAX_TICKS_TO_TRANSMIT - count));
	return true;
}

std::pair<int,int> CPlayerInputsSynchronizer::LoadPaket(char* buff, size_t size, RingBuffer<CPlayerInput>& playerInputsBuffer)
{
	if(size > sizeof PlayerInputsSynchronizerPacketBytesUnion)
	{
		gEnv->pLog->LogToFile("Attempted process packet of size %llu which is bigger than the maximum size of PlayerInputsSynchronizerPacket: %llu", size, sizeof PlayerInputsSynchronizerPacketBytesUnion);	
		return std::pair{m_lastTickAcked.value_or(0), 0};
	}

	const PlayerInputsSynchronizerPacketBytesUnion* packet = reinterpret_cast<PlayerInputsSynchronizerPacketBytesUnion*>(buff);

	const size_t expectedSize = sizeof PlayerInputsSynchronizerPacket - (sizeof packet->packet.Inputs[0] * (MAX_TICKS_TO_TRANSMIT - packet->packet.TickCount));
	if (size != expectedSize)
	{
		gEnv->pLog->LogToFile("Attempted process packet of size %llu which is bigger than the Expected size of a PlayerInputsSynchronizerPacket with TickCount %i : %llu", size, packet->packet.TickCount, sizeof PlayerInputsSynchronizerPacketBytesUnion);
		return std::pair{m_lastTickAcked.value_or(0), 0};
	}

	if (!packet->packet.TickNum.has_value())
		return std::pair{m_lastTickAcked.value_or(0), 0};

	const int maxCount = packet->packet.TickCount;
	if (maxCount > MAX_TICKS_TO_TRANSMIT)
	{
		gEnv->pLog->LogToFile("Attempted to receive %i inputs, which is more than MAX_TICKS_TO_TRANSMIT = %i", maxCount, MAX_TICKS_TO_TRANSMIT);
		return std::pair{m_lastTickAcked.value_or(0), 0};
	}

	const int firstTickToReceive = m_lastTickAcked.has_value() ? m_lastTickAcked.value() + 1 : 0;
	const int lastTickToReceive = packet->packet.TickNum.value();

	const int count = (lastTickToReceive - firstTickToReceive) + 1;

	if (count > maxCount)
	{
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		gEnv->pLog->LogToFile("Attempted to load ticks from %i to %i (inclusive) but packet only contains %i ticks", firstTickToReceive, lastTickToReceive, maxCount);
		return std::pair{m_lastTickAcked.value_or(0), 0};
	}

	const int skip = maxCount - count; // skip over inputs already received in previous packets
	for (int i = 0, j = skip; i < count; i++)
	{
		*(playerInputsBuffer.GetAt(firstTickToReceive+i)) = packet->packet.Inputs[j++];
	}

	m_lastTickAcked.I = lastTickToReceive;

	return std::pair{lastTickToReceive, count};
}
