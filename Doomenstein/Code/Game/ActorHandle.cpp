#include "Game/ActorHandle.hpp"

const ActorHandle ActorHandle::INVALID = ActorHandle(0x0000FFFFu, 0x0000FFFFu);


ActorHandle::ActorHandle(unsigned int uid, unsigned int index)
{
	m_data = (uid << 16) | (0x0000FFFFu & index);
}

bool ActorHandle::IsValid() const
{
	return (*this) != INVALID;
}

unsigned int ActorHandle::GetIndex() const
{
	return m_data & 0x0000FFFFu;
}

bool ActorHandle::operator==(const ActorHandle& other) const
{
	return m_data == other.m_data;
}

bool ActorHandle::operator!=(const ActorHandle& other) const
{
	return m_data != other.m_data;
}


