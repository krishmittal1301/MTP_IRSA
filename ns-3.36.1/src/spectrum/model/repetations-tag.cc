#include "repetations-tag.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("repetationsTag");

repetationsTag::repetationsTag ()
  : m_repetations (0), m_slots () {}

repetationsTag::repetationsTag (int id, const std::vector<int>& values)
  : m_repetations (id), m_slots (values) {}

TypeId
repetationsTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::repetationsTag")
    .SetParent<Tag> ()
    .AddConstructor<repetationsTag> ();
  return tid;
}

TypeId
repetationsTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
repetationsTag::GetSerializedSize (void) const
{
  // size = int (4 bytes) + vector size (4 bytes) + contents
  return 4 + 4 + m_slots.size () * 4;
}

void
repetationsTag::Serialize (TagBuffer i) const
{
  i.WriteU32 (m_repetations);
  i.WriteU32 (m_slots.size ());
  for (size_t j = 0; j < m_slots.size (); ++j)
    {
      i.WriteU32 (m_slots[j]);
    }
}

void
repetationsTag::Deserialize (TagBuffer i)
{
  m_repetations = i.ReadU32 ();
  uint32_t len = i.ReadU32 ();
  m_slots.clear ();
  for (uint32_t j = 0; j < len; ++j)
    {
      m_slots.push_back (i.ReadU32 ());
    }
}

void
repetationsTag::Print (std::ostream &os) const
{
  os << "ID=" << m_repetations << ", Values=[";
  for (size_t j = 0; j < m_slots.size (); ++j)
    {
      os << m_slots[j];
      if (j + 1 < m_slots.size ())
        os << ",";
    }
  os << "]";
}

void
repetationsTag::SetData (int id, const std::vector<int>& values)
{
  m_repetations = id;
  m_slots = values;
}

std::pair<int, std::vector<int>>
repetationsTag::GetData () const
{
  return {m_repetations, m_slots};
}

} // namespace ns3
