/* -*- Mode: C++; indent-tabs-mode: nil; c-basic-offset: 2; tab-width: 2 -*- */
/**
 * @file bus.hpp
 * @author Sandro Stiller
 * @date 2024-10-05
 */
#ifndef DH_GST_BUS_HPP
#define DH_GST_BUS_HPP

#include "object.hpp"
#include "sharedptrs.hpp"

// boost
#include <boost/signals2.hpp>

namespace bs2 = boost::signals2;

namespace dh ::gst
{

class Bus : public Object
{
protected:
  /**
   * @brief Create a new Bus object that wraps a GstBusSPtr.
   * @param gstBus
   */
  explicit Bus(GstBusSPtr gstBus);

  /**
   * @brief Create a new Bus object that wraps a GstBus.
   * @ref makeGstSharedPtr is used to wrap in a internal shared_ptr
   * @param gstBus
   * @param transferType see if None, then increase use count
   */
  Bus(GstBus* gstBus, TransferType transferType);

public:
  [[nodiscard]] static std::shared_ptr<Bus> create(GstBusSPtr gstBus);
  [[nodiscard]] static std::shared_ptr<Bus> create(GstBus* gstBus, TransferType transferType);

  /**
   * @brief get the GstBusSPtr of the Bus
   * @return the internal GstBus.
   */
  [[nodiscard]] GstBusSPtr getGstBus();
  [[nodiscard]] const GstBusSPtr getGstBus() const;

  /**
   * @brief Posts a message on the bus.
   */
  void post(const GstMessageSPtr message);

  /**
   * @brief  A message has been posted on the bus.
   * This signal is emitted from the thread that posted the message so one has to be careful with locking.
   * @todo Calling this function creates a new glib signal connection, no matter if that was already done.
   *       This could be improved by keeping track of bs2 signals (in GObject data)
   */
  [[nodiscard]] bs2::signal<void(GstMessageSPtr)>& newSyncMessageSignal() const;

  // TODO: Enable "message" also? (only valid with existing glib main loop)
private:
  [[nodiscard]] GstBus* getRawGstBus();
  [[nodiscard]] const GstBus* getRawGstBus() const;

};


} // dh::gst

#endif //DH_GST_BUS_HPP
