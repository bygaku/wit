/**
 * Created by intwi on 2026/07/05.
 * Copyright (c) 2026 All rights reserved.
 */
#ifndef WIT_CUE_PLAYER_REGISTER_H
#define WIT_CUE_PLAYER_REGISTER_H

#include <cstddef>
#include <cstdint>
#include <memory>
#include <unordered_map>

#include <wit_types.h>

#include "util/handle.h"

namespace wit {

class CuePlayer;

/**
 * @class CuePlayerRegister
 * @brief Owns CuePlayer instances and issues opaque WitCuePlayerHn handles.
 */
class CuePlayerRegister {
public:
    CuePlayerRegister()  = default;
    ~CuePlayerRegister() = default;

#pragma region prohibit
    CuePlayerRegister(const CuePlayerRegister&)            = delete;
    CuePlayerRegister& operator=(const CuePlayerRegister&) = delete;
    CuePlayerRegister(CuePlayerRegister&&)                 = delete;
    CuePlayerRegister& operator=(CuePlayerRegister&&)      = delete;
#pragma endregion

    /**
     * @brief Create a new CuePlayer and register it.
     */
    WitCuePlayerHn Create();

    /**
     * @brief Destroy the CuePlayer bound to handle. Silent no-op for invalid or already-destroyed handles.
     */
    void Destroy(WitCuePlayerHn handle) noexcept;

    /**
     * @brief Resolve handle to a CuePlayer pointer.
     * @retval CuePlayer*
     * @retval nullptr Invalid or stale handles.
     */
    CuePlayer*       Find(WitCuePlayerHn handle)	   noexcept;

	/**
	 * @brief Resolve handle to a CuePlayer pointer.
	 * @note Read-only
	 * @retval CuePlayer*
	 * @retval nullptr Invalid or stale handles.
	 */
	const CuePlayer* Find(WitCuePlayerHn handle) const noexcept;

    /**
     * @brief Count the number of registered players.
     */
    size_t Count() const noexcept { return players_.size(); }

private:
    HandleAllocator<WitCuePlayerHn>									allocator_;
    std::unordered_map<WitCuePlayerHn, std::unique_ptr<CuePlayer>>	players_;
};

}

#endif // WIT_CUE_PLAYER_REGISTER_H
