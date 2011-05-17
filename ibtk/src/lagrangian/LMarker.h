// Filename: LMarker.h
// Created on 12 Sep 2007 by Boyce Griffith
//
// Copyright (c) 2002-2010, Boyce Griffith
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of New York University nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef included_LMarker
#define included_LMarker

/////////////////////////////// INCLUDES /////////////////////////////////////

// SAMRAI INCLUDES
#include <Index.h>
#include <tbox/AbstractStream.h>
#include <tbox/DescribedClass.h>
#include <tbox/Pointer.h>

// BLITZ++ INCLUDES
#include <blitz/tinyvec.h>

/////////////////////////////// CLASS DEFINITION /////////////////////////////

namespace IBTK
{
/*!
 * \brief Class LMarker provides inter-processor communications functionality
 * for a Lagrangian marker.
 */
class LMarker
    : public SAMRAI::tbox::DescribedClass
{
public:
    /*!
     * \brief Default constructor.
     */
    LMarker(
        const int idx=-1,
        const blitz::TinyVector<double,NDIM>& X=blitz::TinyVector<double,NDIM>(0.0),
        const blitz::TinyVector<double,NDIM>& U=blitz::TinyVector<double,NDIM>(0.0),
        const SAMRAI::hier::IntVector<NDIM>& periodic_offset=SAMRAI::hier::IntVector<NDIM>(0));

    /*!
     * \brief Copy constructor.
     *
     * \param from The value to copy to this object.
     */
    LMarker(
        const LMarker& from);

    /*!
     * \brief Virtual destructor.
     */
    virtual ~LMarker();

    /*!
     * \brief Assignment operator.
     *
     * \param that The value to assign to this object.
     *
     * \return A reference to this object.
     */
    LMarker&
    operator=(
        const LMarker& that);

    /*!
     * \return A const reference to the marker index.
     */
    const int&
    getIndex() const;

    /*!
     * \return A non-const reference to the marker index.
     */
    int&
    getIndex();

    /*!
     * \brief Set the marker index.
     */
    void
    setIndex(
        const int& idx);

    /*!
     * \return A const reference to the marker position.
     */
    const blitz::TinyVector<double,NDIM>&
    getPosition() const;

    /*!
     * \return A non-const reference to the marker position.
     */
    blitz::TinyVector<double,NDIM>&
    getPosition();

    /*!
     * \brief Set the marker position.
     */
    void
    setPosition(
        const blitz::TinyVector<double,NDIM>& X);

    /*!
     * \return A const reference to the marker velocity.
     */
    const blitz::TinyVector<double,NDIM>&
    getVelocity() const;

    /*!
     * \return A non-const reference to the marker velocity.
     */
    blitz::TinyVector<double,NDIM>&
    getVelocity();

    /*!
     * \brief Set the marker velocity.
     */
    void
    setVelocity(
        const blitz::TinyVector<double,NDIM>& U);

    /*!
     * \return A const reference to the periodic offset.
     *
     * \note If the LMarker lives in cell i, the index of the source object is
     * src_index = i - offset.
     */
    const SAMRAI::hier::IntVector<NDIM>&
    getPeriodicOffset() const;

    /*!
     * \brief Set the value of the periodic offset.
     *
     * \note If the LMarker lives in cell i, the index of the source object is
     * src_index = i - offset.
     */
    void
    setPeriodicOffset(
        const SAMRAI::hier::IntVector<NDIM>& offset);

    /*!
     * \brief Copy data from the source.
     *
     * \note The index of the destination object is src_index + src_offset.
     */
    virtual void
    copySourceItem(
        const SAMRAI::hier::Index<NDIM>& src_index,
        const SAMRAI::hier::IntVector<NDIM>& src_offset,
        const LMarker& src_item);

    /*!
     * \brief Return an upper bound on the amount of space required to pack the
     * object to a buffer.
     */
    virtual size_t
    getDataStreamSize() const;

    /*!
     * \brief Pack data into the output stream.
     */
    virtual void
    packStream(
        SAMRAI::tbox::AbstractStream& stream);

    /*!
     * \brief Unpack data from the input stream.
     */
    virtual void
    unpackStream(
        SAMRAI::tbox::AbstractStream& stream,
        const SAMRAI::hier::IntVector<NDIM>& offset);

private:
    /*!
     * \brief The marker index.
     */
    int d_idx;

    /*!
     * \brief The marker position and velocity.
     */
    blitz::TinyVector<double,NDIM> d_X, d_U;

    /*!
     * \brief The periodic offset.
     */
    SAMRAI::hier::IntVector<NDIM> d_offset;
};
}// namespace IBTK

/////////////////////////////// INLINE ///////////////////////////////////////

#include <ibtk/LMarker.I>

//////////////////////////////////////////////////////////////////////////////

#endif //#ifndef included_LMarker