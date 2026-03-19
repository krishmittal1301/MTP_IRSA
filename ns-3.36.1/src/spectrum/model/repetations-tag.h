#ifndef PAIR_VECTOR_TAG_H
#define PAIR_VECTOR_TAG_H

#include "ns3/tag.h"
#include "ns3/uinteger.h"
#include <vector>
#include <utility>

namespace ns3 {

class repetationsTag : public Tag
{
public:
  repetationsTag ();
  repetationsTag (int id, const std::vector<int>& values);

  // Required ns-3 functions
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;

  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  // Custom setters/getters
  void SetData (int id, const std::vector<int>& values);
  std::pair<int, std::vector<int>> GetData () const;

private:
  int m_repetations;
  std::vector<int> m_slots;
};

} // namespace ns3

#endif // PAIR_VECTOR_TAG_H
