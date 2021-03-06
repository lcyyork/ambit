/*
 * @BEGIN LICENSE
 *
 * ambit: C++ library for the implementation of tensor product calculations
 *        through a clean, concise user interface.
 *
 * Copyright (c) 2014-2017 Ambit developers.
 *
 * The copyrights for code used from other parties are included in
 * the corresponding files.
 *
 * This file is part of ambit.
 *
 * Ambit is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Ambit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License along
 * with ambit; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * @END LICENSE
 */

//
// Created by Justin Turney on 1/11/16.
//

#include <ambit/io/hdf5/dataspace.h>

namespace ambit {

namespace io {

namespace hdf5 {

Dataspace::Dataspace(const Tensor& tensor)
{
    vector<hsize_t> cdims(tensor.dims().begin(), tensor.dims().end());
    dataspace_id_ = H5Screate_simple(static_cast<int>(cdims.size()),
                                     cdims.data(),
                                     nullptr);

    if (dataspace_id_ < 0) {
        throw std::runtime_error("Unable to create dataspace.");
    }
}

Dataspace::Dataspace(const Dimension& current_dims)
{
    assert(current_dims.size() > 0);

    vector<hsize_t> cdims(current_dims.begin(), current_dims.end());
    dataspace_id_ = H5Screate_simple(static_cast<int>(cdims.size()),
                                     cdims.data(),
                                     nullptr);

    if (dataspace_id_ < 0) {
        throw std::runtime_error("Unable to create dataspace.");
    }
}

Dataspace::Dataspace(const Dimension& current_dims, const Dimension& maximum_dims)
{
    assert(current_dims.size() > 0);
    assert(current_dims.size() == maximum_dims.size());

    vector<hsize_t> cdims(current_dims.begin(), current_dims.end());
    vector<hsize_t> mdims(maximum_dims.begin(), maximum_dims.end());

    dataspace_id_ = H5Screate_simple(static_cast<int>(cdims.size()),
                                     cdims.data(),
                                     mdims.data());

    if (dataspace_id_ < 0) {
        throw std::runtime_error("Unable to create dataspace.");
    }
}

Dataspace::~Dataspace()
{
    if (dataspace_id_ >= 0)
        H5Sclose(dataspace_id_);
    dataspace_id_ = -1;
}

} // namespace hdf5

} // namespace io

} // namespace ambit
