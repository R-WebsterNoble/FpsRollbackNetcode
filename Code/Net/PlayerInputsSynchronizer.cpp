#include "PlayerInputsSynchronizer.h"


void CPlayerInputsSynchronizer::Enqueue(int tickNum, const CPlayerInput &playerInput)
{
	m_tickNum.I = tickNum;
	(*m_playerInputsBuffer.GetAt(tickNum)) = playerInput;
}

void CPlayerInputsSynchronizer::Ack(const OptInt ack)
{
	m_lastTickAcked.I = ack.I;
}

bool CPlayerInputsSynchronizer::GetPaket(flatbuffers::FlatBufferBuilder& builder, OUT flatbuffers::Offset<FlatBuffPacket::PlayerInputsSynchronizer>& synchronizer)
{

	if(!m_tickNum.has_value())
	{
		const FlatBuffPacket::OptInt optInt(m_tickNum.I);
		synchronizer = CreatePlayerInputsSynchronizer(builder, &optInt, 0);
		return true;
	}

	const int firstTickToSend = m_lastTickAcked.has_value() ? m_lastTickAcked.value() + 1 : 0;
	const int lastTickToSend = m_tickNum.value();
	const int count = (lastTickToSend - firstTickToSend) + 1;

	if (count > MAX_TICKS_TO_TRANSMIT)
	{
		gEnv->pLog->LogToFile("Attempted Send %i inputs but max is %i", count, MAX_TICKS_TO_TRANSMIT);
		return false;
	}

	// packet->packet.TickNum.I = lastTickToSend;
	// packet->packet.TickCount = count;	
	//
	// for (int i = 0; i < count; ++i)
	// {
	// 	packet->packet.Inputs[i] = *(m_playerInputsBuffer.GetAt(firstTickToSend+i));
	// }
	FlatBuffPacket::PlayerInput playerInputs[MAX_TICKS_TO_TRANSMIT];
	//std::vector<FlatBuffPacket::PlayerInput> playerInputs = std::vector<FlatBuffPacket::PlayerInput>(count);
	
	std::vector<flatbuffers::Offset<FlatBuffPacket::PlayerInput>> inputs;
	for (int i = 0; i < count; ++i)
	{
		// packet->packet.Inputs[i] = *(m_playerInputsBuffer.GetAt(firstTickToSend + i));

		const CPlayerInput* val = m_playerInputsBuffer.GetAt(firstTickToSend + i);

		FlatBuffPacket::Vec2 mouseDelta = FlatBuffPacket::Vec2(val->mouseDelta.x, val->mouseDelta.y);
		const FlatBuffPacket::InputFlags playerActions = static_cast<FlatBuffPacket::InputFlags>(static_cast<int8_t>(val->playerActions));
		const FlatBuffPacket::PlayerInput playerInput = FlatBuffPacket::PlayerInput(mouseDelta, playerActions);
		//playerInputs.push_back(playerInput);
		playerInputs[i] = playerInput;
	}
	const flatbuffers::Offset<flatbuffers::Vector<const FlatBuffPacket::PlayerInput*>> inputsFlatBuff = builder.CreateVectorOfStructs(playerInputs, count);
	const FlatBuffPacket::OptInt optInt(m_tickNum.I);
	synchronizer = CreatePlayerInputsSynchronizer(builder, &optInt, count, inputsFlatBuff);

	return true;
}

void CPlayerInputsSynchronizer::UpdateFromPacket(const FlatBuffPacket::PlayerInputsSynchronizer* sync)
{
	std::tuple<int, int, std::vector<CPlayerInput>> tuple = ParsePaket(sync, m_lastTickAcked);
	auto [firstTickNum, count, thing] = tuple;
	
	for (int i = 0; i < count; ++i)
	{
		Enqueue(firstTickNum + i, thing[i]);
	}
}

std::tuple<int, int, std::vector<CPlayerInput>> CPlayerInputsSynchronizer::ParsePaket(const FlatBuffPacket::PlayerInputsSynchronizer* sync, AtomicOptInt& lastTickAcked)
{
	const OptInt tickCount(sync->tick_count());
	if (!tickCount.has_value())
		return std::make_tuple(0, 0, std::vector<CPlayerInput>(0));

	const int maxCount = tickCount.value();
	if (maxCount > MAX_TICKS_TO_TRANSMIT)
	{
		gEnv->pLog->LogToFile("Attempted to receive %i inputs, which is more than MAX_TICKS_TO_TRANSMIT = %i", maxCount, MAX_TICKS_TO_TRANSMIT);
		return std::make_tuple(0, 0, std::vector<CPlayerInput>(0));
	}

	const int firstTickToReceive = lastTickAcked.has_value() ? lastTickAcked.value() + 1 : 0;
	const int lastTickToReceive = OptInt(sync->tick_num()->i()).value_or(0);

	int count = (lastTickToReceive - firstTickToReceive) + 1;

	if (count > maxCount || count > sync->inputs()->size())
	{
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);
		gEnv->pLog->LogToFile("Attempted to load ticks from %i to %i (inclusive) but packet only contains %i ticks", firstTickToReceive, lastTickToReceive, maxCount);
		return std::make_tuple(0, 0, std::vector<CPlayerInput>(0));
	}

	const int skip = maxCount - count; // skip over inputs already received in previous packets
	
	std::vector<CPlayerInput> rPlayerInputs = std::vector<CPlayerInput>(count);
	for (int i = 0; i < count; ++i)
	{
		const FlatBuffPacket::PlayerInput* playerInput = sync->inputs()->Get(skip+i);
		rPlayerInputs[i] = (CPlayerInput{ Vec2{playerInput->mouse_delta().x(), playerInput->mouse_delta().y()}, static_cast<EInputFlag>(static_cast<uint8_t>(playerInput->player_actions())) });
	}


	lastTickAcked.I = lastTickToReceive;

	return std::make_tuple(firstTickToReceive, count, rPlayerInputs);
}