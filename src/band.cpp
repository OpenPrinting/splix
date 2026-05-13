/*
 * 	    band.cpp                  (C) 2006-2008, Aurélien Croc (AP²C)
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
#include "band.h"
#include "sp_portable.h"
#include "errlog.h"
#include "bandplane.h"
#include "page.h"

/*
 * Constructeur - Destructeur
 * Init - Uninit 
 */
Band::Band() = default;
Band::Band (uint32_t nr, uint32_t width, uint32_t height)
    : _bandNr(nr), _width(width), _height(height) {}
Band::~Band() = default;


void Band::registerPlane(std::unique_ptr<BandPlane> plane)
{
    if (_planes.size() < 4) {
        _planes.push_back(std::move(plane));
    }
}


/*
 * Mise sur disque / Rechargement
 * Swapping / restoring
 */
SP::Result<> Band::swapToDisk(int fd)
{
    uint32_t colors = static_cast<uint32_t>(_planes.size());
    if (write(fd, &_bandNr, sizeof(_bandNr)) == -1) return SP::Unexpected(SP::Error::IOError);
    if (write(fd, &colors, sizeof(colors)) == -1) return SP::Unexpected(SP::Error::IOError);
    if (write(fd, &_width, sizeof(_width)) == -1) return SP::Unexpected(SP::Error::IOError);
    if (write(fd, &_height, sizeof(_height)) == -1) return SP::Unexpected(SP::Error::IOError);
    
    for (auto &plane : _planes) {
        auto res = plane->swapToDisk(fd);
        if (!res) return res;
    }
    return {};
}

SP::Result<std::unique_ptr<Band>> Band::restoreIntoMemory(int fd)
{
    uint32_t colors = 0;
    auto band = std::make_unique<Band>();

    if (read(fd, &band->_bandNr, sizeof(band->_bandNr)) <= 0) return SP::Unexpected(SP::Error::IOError);
    if (read(fd, &colors, sizeof(colors)) <= 0) return SP::Unexpected(SP::Error::IOError);
    if (read(fd, &band->_width, sizeof(band->_width)) <= 0) return SP::Unexpected(SP::Error::IOError);
    if (read(fd, &band->_height, sizeof(band->_height)) <= 0) return SP::Unexpected(SP::Error::IOError);

    // Safety check: SpliX supports up to 4 colors (CMYK)
    if (colors > 4) {
        return SP::Unexpected(SP::Error::InconsistentData);
    }

    for (uint32_t i = 0; i < colors; i++) {
        auto res = BandPlane::restoreIntoMemory(fd);
        if (!res) {
            return SP::Unexpected(res.error());
        }
        band->registerPlane(std::move(res.value()));
    }

    return std::move(band);
}

/* vim: set expandtab tabstop=4 shiftwidth=4 smarttab tw=80 cin enc=utf8: */

