// Copyright (c) 2021  Lukasz Chodyla
// Distributed under the Boost Software License v1.0.
// See accompanying file LICENSE.txt or copy at
// https://www.boost.org/LICENSE_1_0.txt for the full license.

#ifndef OBYAMLCONFIGREADER_H
#define OBYAMLCONFIGREADER_H

//struct ObContext;
typedef struct ObContext ObContext;

void obLoadYamlConfig(ObContext* context, const char* path);

#endif // OBYAMLCONFIGREADER_H
