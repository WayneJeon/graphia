/* Copyright © 2013-2020 Graphia Technologies Ltd.
 *
 * This file is part of Graphia.
 *
 * Graphia is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Graphia is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Graphia.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef COLUMNANNOTATION_H
#define COLUMNANNOTATION_H

#include <QString>

#include <vector>
#include <map>

class ColumnAnnotation
{
private:
    QString _name;

    std::vector<QString> _values;
    std::map<QString, int> _uniqueValues;

public:
    using Iterator = std::vector<QString>::const_iterator;

    ColumnAnnotation(QString name, std::vector<QString> values);
    ColumnAnnotation(QString name, const Iterator& begin, const Iterator& end);

    const QString& name() const { return _name; }

    const QString& valueAt(size_t index) const { return _values.at(index); }
    int uniqueIndexOf(const QString& value) const;
};

#endif // COLUMNANNOTATION_H
