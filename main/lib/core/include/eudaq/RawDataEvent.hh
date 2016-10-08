#ifndef EUDAQ_INCLUDED_RawDataEvent
#define EUDAQ_INCLUDED_RawDataEvent

#include <sstream>
#include <string>
#include <vector>
#include "Event.hh"
#include "Platform.hh"
namespace eudaq {

  class DLLEXPORT RawDataEvent : public Event {
  public:
    struct DLLEXPORT block_t : public Serializable {
      block_t() = default;
      block_t(uint32_t id, const std::vector<uint8_t> &data)
          : id(id), data(data) {}
      block_t(Deserializer &);
      void Serialize(Serializer &) const;
      void Append(const std::vector<uint8_t> &data);
      uint32_t id;
      std::vector<uint8_t> data;
    };

    RawDataEvent(std::string type, uint32_t id_stream, unsigned run, unsigned event);
    RawDataEvent(Deserializer &);
    virtual void Serialize(Serializer &) const;
    virtual void Print(std::ostream & ,size_t offset = 0) const;

    uint32_t GetID(size_t i) const { return m_blocks.at(i).id; }
    const std::vector<uint8_t>& GetBlock(size_t i) const;
    size_t NumBlocks() const { return m_blocks.size(); }

    virtual std::string GetSubType() const { return m_type; }
    void SetSubType(std::string subtype){ m_type = subtype; }

    
    /// Add a data block as std::vector
    template <typename T>
    size_t AddBlock(unsigned id, const std::vector<T> &data) {
      m_blocks.push_back(block_t(id, make_vector(data)));
      return m_blocks.size() - 1;
    }

    /// Add a data block as array with given size
    template <typename T>
    size_t AddBlock(unsigned id, const T *data, size_t bytes) {
      m_blocks.push_back(block_t(id, make_vector(data, bytes)));
      return m_blocks.size() - 1;
    }

    /// Append data to a block as std::vector
    template <typename T>
    void AppendBlock(size_t index, const std::vector<T> &data) {
      m_blocks[index].Append(make_vector(data));
    }

    /// Append data to a block as array with given size
    template <typename T>
    void AppendBlock(size_t index, const T *data, size_t bytes) {
      m_blocks[index].Append(make_vector(data, bytes));
    }
    
  private:
    template <typename T>
      static std::vector<uint8_t> make_vector(const T *data, size_t bytes) {
      const uint8_t *ptr = reinterpret_cast<const uint8_t *>(data);
      return std::vector<uint8_t>(ptr, ptr + bytes);
    }

    template <typename T>
    static std::vector<uint8_t> make_vector(const std::vector<T> &data) {
      const uint8_t *ptr = reinterpret_cast<const uint8_t *>(&data[0]);
      return std::vector<uint8_t>(ptr, ptr + data.size() * sizeof(T));
    }

    std::string m_type;
    std::vector<block_t> m_blocks;
  };
}

#endif // EUDAQ_INCLUDED_RawDataEvent
