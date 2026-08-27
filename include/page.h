// SPDX-License-Identifier: GPL-2.0-only
// Copyright 2006-2008 Aurélien Croc and SpliX contributors
#ifndef _PAGE_H_
#define _PAGE_H_
#include <memory>
#include <vector>
#include <array>
#include <cstdint>
#include "sp_result.h"
class Band;
class Page
{
    protected:
        uint32_t                            _xResolution = 0;
        uint32_t                            _yResolution = 0;
        uint32_t                            _width = 0;
        uint32_t                            _height = 0;
        uint8_t                             _colors = 0;
        uint32_t                            _pageNr = 0;
        uint32_t                            _copiesNr = 0;
        uint32_t                            _compression = 0;
        std::array<std::vector<uint8_t>, 4> _planes;
        bool                                _empty = true;
        uint32_t                            _bandsNr = 0;
        std::vector<uint8_t>                _bih;
        std::unique_ptr<Band>               _firstBand;
        Band*                               _lastBand = nullptr; // Non-owning observer
    public:
        Page();
        virtual ~Page();
    public:
        long double             convertToXResolution(long double f) const
                                    {return f * _xResolution / 72.;}
        long double             convertToYResolution(long double f) const
                                    {return f * _yResolution / 72.;}
        void                    flushPlanes();
        void                    rotate();
    public:
        void                    setXResolution(uint32_t xRes)
                                    {_xResolution = xRes;}
        void                    setYResolution(uint32_t yRes)
                                    {_yResolution = yRes;}
        void                    setWidth(uint32_t width)
                                    {_width = width;}
        void                    setHeight(uint32_t height)
                                    {_height = height;}
        void                    setColorsNr(uint8_t nr) {_colors = nr;}
        void                    setPageNr(uint32_t nr) {_pageNr = nr;}
        void                    setCopiesNr(uint32_t nr)
                                    {_copiesNr = nr;}
        void                    setCompression(uint32_t nr)
                                    {_compression = nr;}
        SP::Result<>            setPlaneBuffer(uint8_t color,
                                    std::vector<uint8_t> buffer);
        void                    registerBand(std::unique_ptr<Band> band);
        void                    setEmpty() {_empty = true;}
        uint32_t                xResolution() const {return _xResolution;}
        uint32_t                yResolution() const {return _yResolution;}
        uint32_t                width() const {return _width;}
        uint32_t                height() const {return _height;}
        uint8_t                 colorsNr() const {return _colors;}
        uint32_t                bandsNr() const {return _bandsNr;}
        uint32_t                pageNr() const {return _pageNr;}
        uint32_t                copiesNr() const {return _copiesNr;}
        uint32_t                compression() const {return _compression;}
        uint8_t*                planeBuffer(uint8_t color)
                                    {return color < _colors && !_planes[color].empty()
                                        ? _planes[color].data() : nullptr;}
        const uint8_t*          planeBuffer(uint8_t color) const
                                    {return color < _colors && !_planes[color].empty()
                                        ? _planes[color].data() : nullptr;}
        bool                    isEmpty() const {return _empty;}
        const Band*             firstBand() const {return _firstBand.get();}
    public:
        SP::Result<>            swapToDisk(int fd);
        static SP::Result<std::unique_ptr<Page>> restoreIntoMemory(int fd);
        void                    setBIH(const uint8_t *bih_data, size_t size = 20);
        const uint8_t*          getBIH() const { return _bih.empty() ? nullptr : _bih.data(); }
};
#endif /* _PAGE_H_ */
