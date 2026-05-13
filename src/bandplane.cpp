/*
 * 	    bandplane.cpp             (C) 2006-2008, Aurélien Croc (AP²C)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; version 2 of the License.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the
 *  Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *  $Id$
 * 
 */
#include "bandplane.h"
#include "sp_portable.h"
#include <numeric>

/*
 * Constructeur - Destructeur
 * Init - Uninit 
 */
// LIFECYCLE: Standard constructors and destructors are defaulted in the header via member initializers.


/*
 * Enregistrement des données
 * Set data
 */
void BandPlane::setData(std::vector<uint8_t> data)
{
    _data = std::move(data);
    _checksum = std::accumulate(_data.begin(), _data.end(), 0U);
}



/*
 * Mise sur disque / Rechargement
 * Swapping / restoring
 */
namespace {
    /**
     * @brief Helper for robustly writing a fixed-size buffer.
     */
    bool writeAll(int fd, const void* data, size_t size) noexcept {
        const uint8_t* ptr = static_cast<const uint8_t*>(data);
        while (size > 0) {
            ssize_t written = write(fd, ptr, size);
            if (written <= 0) return false;
            ptr += written;
            size -= written;
        }
        return true;
    }

    /**
     * @brief Helper for robustly reading a fixed-size buffer.
     */
    bool readAll(int fd, void* data, size_t size) noexcept {
        uint8_t* ptr = static_cast<uint8_t*>(data);
        while (size > 0) {
            ssize_t bytesRead = read(fd, ptr, size);
            if (bytesRead <= 0) return false;
            ptr += bytesRead;
            size -= bytesRead;
        }
        return true;
    }
}

SP::Result<> BandPlane::swapToDisk(int fd)
{
    const size_t size = _data.size();
    
    if (!writeAll(fd, &_colorNr, sizeof(_colorNr))) return SP::Unexpected(SP::Error::IOError);
    if (!writeAll(fd, &size, sizeof(size))) return SP::Unexpected(SP::Error::IOError);
    if (!writeAll(fd, _data.data(), size)) return SP::Unexpected(SP::Error::IOError);
    if (!writeAll(fd, &_checksum, sizeof(_checksum))) return SP::Unexpected(SP::Error::IOError);
    if (!writeAll(fd, &_endian, sizeof(_endian))) return SP::Unexpected(SP::Error::IOError);
    if (!writeAll(fd, &_compression, sizeof(_compression))) return SP::Unexpected(SP::Error::IOError);
    
    return {};
}

SP::Result<std::unique_ptr<BandPlane>> BandPlane::restoreIntoMemory(int fd)
{
    auto plane = std::make_unique<BandPlane>();
    size_t size = 0;

    if (!readAll(fd, &plane->_colorNr, sizeof(plane->_colorNr))) return SP::Unexpected(SP::Error::IOError);
    if (!readAll(fd, &size, sizeof(size))) return SP::Unexpected(SP::Error::IOError);
    
    // Safety check: 512MB cap for cached planes
    if (size > 1024 * 1024 * 512) {
        return SP::Unexpected(SP::Error::InconsistentData);
    }

    try {
        plane->_data.resize(size);
    } catch (const std::bad_alloc&) {
        return SP::Unexpected(SP::Error::MemoryError);
    }

    if (!readAll(fd, plane->_data.data(), size)) return SP::Unexpected(SP::Error::IOError);
    
    if (!readAll(fd, &plane->_checksum, sizeof(plane->_checksum))) return SP::Unexpected(SP::Error::IOError);
    if (!readAll(fd, &plane->_endian, sizeof(plane->_endian))) return SP::Unexpected(SP::Error::IOError);
    if (!readAll(fd, &plane->_compression, sizeof(plane->_compression))) return SP::Unexpected(SP::Error::IOError);

    return plane;
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */
