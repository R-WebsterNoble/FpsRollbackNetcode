#include "PlayerInputsSynchronizer.h"



void CPlayerInputsSynchronizerSender::Enqueue(int tickNum, CPlayerInput playerInput)
{
	m_tickNum.I = tickNum;
	(*m_playerInputsBuffer.GetAt(tickNum)) = playerInput;
}

bool CPlayerInputsSynchronizerSender::GetPaket(OUT char* buff, OUT size_t& size)
{
	PlayerInputsSynchronizerClientSendPacketBytesUnion* packet = reinterpret_cast<PlayerInputsSynchronizerClientSendPacketBytesUnion*>(buff);

	if(!m_tickNum.has_value())
	{
		packet->packet.TickNum = OptInt{};
		packet->packet.TickCount = 0;
		size = sizeof PlayerInputsSynchronizerClientSendPacket - (sizeof packet->packet.Inputs[0] * MAX_TICKS_TO_TRANSMIT);
		return true;
	}

	const int firstTickToSend = m_lastTickAcked.has_value() ? m_lastTickAcked.value() + 1 : 0;
	const int lastTickToSend = m_tickNum.value();
	const int count = (lastTickToSend - firstTickToSend) + 1;

	if (count > MAX_TICKS_TO_TRANSMIT)
	{
		size = 0;
		return false;
	}

	packet->packet.TickNum.I = lastTickToSend;
	packet->packet.TickCount = count;	

	for (int i = 0; i < count; ++i)
	{
		packet->packet.Inputs[i] = *(m_playerInputsBuffer.GetAt(firstTickToSend+i));
	}

	size = sizeof PlayerInputsSynchronizerClientSendPacket - (sizeof packet->packet.Inputs[0] * (MAX_TICKS_TO_TRANSMIT - count));
	return true;
}

void CPlayerInputsSynchronizerSender::Ack(OptInt tickNum)
{
	if(tickNum.has_value())
		m_lastTickAcked = tickNum;
}

OptInt CPlayerInputsSynchronizerReceiver::LoadPaket(char* buff, size_t size, RingBuffer<CPlayerInput>& playerInputsBuffer)
{
	if(size > sizeof PlayerInputsSynchronizerClientSendPacketBytesUnion)
	{
		gEnv->pLog->LogToFile("Attempted process packet of size %llu which is bigger than the maximum size of PlayerInputsSynchronizerClientSendPacket: %llu", size, sizeof PlayerInputsSynchronizerClientSendPacketBytesUnion);
		return m_lastTickReceived;
	}

	const PlayerInputsSynchronizerClientSendPacketBytesUnion* packet = reinterpret_cast<PlayerInputsSynchronizerClientSendPacketBytesUnion*>(buff);

	if (!packet->packet.TickNum.has_value())
		return OptInt{};

	const int maxCount = packet->packet.TickCount;
	if (maxCount > MAX_TICKS_TO_TRANSMIT)
	{
		gEnv->pLog->LogToFile("Attempted to receive %i inputs, which is more than MAX_TICKS_TO_TRANSMIT = %i", maxCount, MAX_TICKS_TO_TRANSMIT);
		return m_lastTickReceived;
	}

	const int firstTickToReceive = m_lastTickReceived.has_value() ? m_lastTickReceived.value() + 1 : 0;
	const int lastTickToReceive = packet->packet.TickNum.value();

	const int count = (lastTickToReceive - firstTickToReceive) + 1;

	if (count > maxCount)
	{
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		gEnv->pLog->LogToFile("Attempted to load ticks from %i to %i (inclusive) but packet only contains %i ticks", firstTickToReceive, lastTickToReceive, maxCount);
		return m_lastTickReceived;
	}

	const int skip = maxCount - count; // skip over inputs already received in previous packets
	for (int i = 0, j = skip; i < count; i++)
	{
		*(playerInputsBuffer.GetAt(firstTickToReceive+i)) = packet->packet.Inputs[j++];
	}

	m_lastTickReceived.I = lastTickToReceive;

	return m_lastTickReceived;
}
