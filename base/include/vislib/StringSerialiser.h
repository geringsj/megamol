/*
 * StringSerialiser.h
 *
 * Copyright (C) 2006 - 2008 by Universitaet Stuttgart (VIS). 
 * Alle Rechte vorbehalten.
 */

#ifndef VISLIB_STRINGSERIALISER_H_INCLUDED
#define VISLIB_STRINGSERIALISER_H_INCLUDED
#if (defined(_MSC_VER) && (_MSC_VER > 1000))
#pragma once
#endif /* (defined(_MSC_VER) && (_MSC_VER > 1000)) */
#if defined(_WIN32) && defined(_MANAGED)
#pragma managed(push, off)
#endif /* defined(_WIN32) && defined(_MANAGED) */


#include "vislib/Array.h"
#include "the/no_such_element_exception.h"
#include "vislib/Pair.h"
#include "vislib/Serialiser.h"
#include "the/string.h"
#include "vislib/StringTokeniser.h"
#include "the/text/string_builder.h"


namespace vislib {

#ifdef _WIN32
#define _I64_PRINTF "I64"
#else /* _WIN32 */
#define _I64_PRINTF "ll"
#endif /* _WIN32 */


    /**
     * Implementation of serialiser operating on a string. This serialiser uses
     * the names and the order, if no name is specified, of the elements to
     * serialise or deserialise 'Serialisable' objects. Be aware of the fact
     * that the strings might be copied often and that this serialiser might be
     * very slow because of this.
     */
    template <class T> class StringSerialiser : public Serialiser {
    public:

        /**
         * Ctor.
         *
         * @param str The default value of the string. This is usually used
         *            for deserialisation. To serialise data, leave this 
         *            parameter 'NULL'.
         */
        StringSerialiser(const typename T::value_type *str = NULL);

        /**
         * Ctor.
         *
         * @param str The value of the string. This is used for 
         *            deserialisation.
         */
        StringSerialiser(const T& str);

        /** Dtor. */
        virtual ~StringSerialiser(void);

        /**
         * Clears the internal data buffer of the serialiser.
         */
        inline void ClearData(void) {
            this->SetInputString(NULL);
        }

        virtual void Deserialise(bool& outValue, 
                const char *name = NULL) {
            outValue = the::text::string_utility::parse_bool(this->value(name));
        }

        virtual void Deserialise(bool& outValue, 
                const wchar_t *name) {
            outValue = the::text::string_utility::parse_bool(this->value(name));
        }

        virtual void Deserialise(wchar_t& outValue, 
                const char *name = NULL) {
            unsigned int u;
            this->Deserialise(u, name);
            outValue = static_cast<wchar_t>(u);
        }

        virtual void Deserialise(wchar_t& outValue, 
                const wchar_t *name) {
            unsigned int u;
            this->Deserialise(u, name);
            outValue = static_cast<wchar_t>(u);
        }

        virtual void Deserialise(int8_t& outValue, 
                const char *name = NULL) {
            outValue = static_cast<int8_t>(the::text::string_utility::parse_int(this->value(name)));
        }

        virtual void Deserialise(int8_t& outValue, 
                const wchar_t *name) {
            outValue = static_cast<int8_t>(the::text::string_utility::parse_int(this->value(name)));
        }

        virtual void Deserialise(uint8_t& outValue, 
                const char *name = NULL) {
            outValue = static_cast<uint8_t>(the::text::string_utility::parse_int(this->value(name)));
        }

        virtual void Deserialise(uint8_t& outValue, 
                const wchar_t *name) {
            outValue = static_cast<uint8_t>(the::text::string_utility::parse_int(this->value(name)));
        }

        virtual void Deserialise(int16_t& outValue, 
                const char *name = NULL) {
            outValue = static_cast<int16_t>(the::text::string_utility::parse_int(this->value(name)));
        }

        virtual void Deserialise(int16_t& outValue, 
                const wchar_t *name) {
            outValue = static_cast<int16_t>(the::text::string_utility::parse_int(this->value(name)));
        }

        virtual void Deserialise(uint16_t& outValue, 
                const char *name = NULL) {
            outValue = static_cast<uint16_t>(the::text::string_utility::parse_int(this->value(name)));
        }

        virtual void Deserialise(uint16_t& outValue, 
                const wchar_t *name) {
            outValue = static_cast<uint16_t>(the::text::string_utility::parse_int(this->value(name)));
        }

        virtual void Deserialise(int32_t& outValue, 
                const char *name = NULL) {
            outValue = static_cast<int32_t>(the::text::string_utility::parse_int(this->value(name)));
        }

        virtual void Deserialise(int32_t& outValue, 
                const wchar_t *name) {
            outValue = static_cast<int32_t>(the::text::string_utility::parse_int(this->value(name)));
        }

        virtual void Deserialise(uint32_t& outValue, 
                const char *name = NULL) {
            outValue = static_cast<uint32_t>(the::text::string_utility::parse_uint64(this->value(name)));
        }

        virtual void Deserialise(uint32_t& outValue, 
                const wchar_t *name) {
            outValue = static_cast<uint32_t>(the::text::string_utility::parse_uint64(this->value(name)));
        }

        virtual void Deserialise(int64_t& outValue, 
                const char *name = NULL) {
            outValue = the::text::string_utility::parse_int64(this->value(name));
        }

        virtual void Deserialise(int64_t& outValue, 
                const wchar_t *name) {
            outValue = the::text::string_utility::parse_int64(this->value(name));
        }

        virtual void Deserialise(uint64_t& outValue, 
                const char *name = NULL) {
            outValue = the::text::string_utility::parse_uint64(this->value(name));
        }

        virtual void Deserialise(uint64_t& outValue, 
                const wchar_t *name) {
            outValue = the::text::string_utility::parse_uint64(this->value(name));
        }

        virtual void Deserialise(float& outValue, 
                const char *name = NULL) {
            outValue = static_cast<float>(the::text::string_utility::parse_double(this->value(name)));
        }

        virtual void Deserialise(float& outValue, 
                const wchar_t *name) {
            outValue = static_cast<float>(the::text::string_utility::parse_double(this->value(name)));
        }

        virtual void Deserialise(double& outValue, 
                const char *name = NULL) {
            outValue = the::text::string_utility::parse_double(this->value(name));
        }

        virtual void Deserialise(double& outValue, 
                const wchar_t *name) {
            outValue = the::text::string_utility::parse_double(this->value(name));
        }

        virtual void Deserialise(the::astring& outValue, 
                const char *name = NULL) {
            the::text::string_converter::convert(outValue, this->value(name));
        }

        virtual void Deserialise(the::astring& outValue, 
                const wchar_t *name) {
            the::text::string_converter::convert(outValue, this->value(name));
        }

        virtual void Deserialise(the::wstring& outValue, 
                const char *name = NULL) {
            the::text::string_converter::convert(outValue, this->value(name));
        }

        virtual void Deserialise(the::wstring& outValue, 
                const wchar_t *name) {
            the::text::string_converter::convert(outValue, this->value(name));
        }

        virtual void Serialise(const bool value, 
                const char *name = NULL) {
            this->Serialise(the::astring(value ? "1" : "0"), name);
        }

        virtual void Serialise(const bool value, 
                const wchar_t *name) {
            this->Serialise(the::astring(value ? "1" : "0"), name);
        }

        virtual void Serialise(const wchar_t value,
                const char *name = NULL) {
            this->Serialise(static_cast<unsigned int>(value), name);
        }

        virtual void Serialise(const wchar_t value,
                const wchar_t *name) {
            this->Serialise(static_cast<unsigned int>(value), name);
        }

        virtual void Serialise(const int8_t value,
                const char *name = NULL) {
            this->Serialise(static_cast<int>(value), name);
        }

        virtual void Serialise(const int8_t value,
                const wchar_t *name) {
            this->Serialise(static_cast<int>(value), name);
        }

        virtual void Serialise(const uint8_t value,
                const char *name = NULL) {
            this->Serialise(static_cast<unsigned int>(value), name);
        }

        virtual void Serialise(const uint8_t value,
                const wchar_t *name) {
            this->Serialise(static_cast<unsigned int>(value), name);
        }

        virtual void Serialise(const int16_t value,
                const char *name = NULL) {
            this->Serialise(static_cast<int>(value), name);
        }

        virtual void Serialise(const int16_t value,
                const wchar_t *name) {
            this->Serialise(static_cast<int>(value), name);
        }

        virtual void Serialise(const uint16_t value,
                const char *name = NULL) {
            this->Serialise(static_cast<unsigned int>(value), name);
        }

        virtual void Serialise(const uint16_t value,
                const wchar_t *name) {
            this->Serialise(static_cast<unsigned int>(value), name);
        }

        virtual void Serialise(const int32_t value,
                const char *name = NULL) {
            the::astring s;
            the::text::astring_builder::format_to(s, "%d", value);
            this->Serialise(s, name);
        }

        virtual void Serialise(const int32_t value,
                const wchar_t *name) {
            the::astring s;
            the::text::astring_builder::format_to(s, "%d", value);
            this->Serialise(s, name);
        }

        virtual void Serialise(const uint32_t value,
                const char *name = NULL) {
            the::astring s;
            the::text::astring_builder::format_to(s, "%u", value);
            this->Serialise(s, name);
        }

        virtual void Serialise(const uint32_t value,
                const wchar_t *name) {
            the::astring s;
            the::text::astring_builder::format_to(s, "%u", value);
            this->Serialise(s, name);
        }

        virtual void Serialise(const int64_t value,
                const char *name = NULL) {
            the::astring s;
            the::text::astring_builder::format_to(s, "%" _I64_PRINTF "d", value);
            this->Serialise(s, name);
        }

        virtual void Serialise(const int64_t value,
                const wchar_t *name) {
            the::astring s;
            the::text::astring_builder::format_to(s, "%" _I64_PRINTF "d", value);
            this->Serialise(s, name);
        }

        virtual void Serialise(const uint64_t value,
                const char *name = NULL) {
            the::astring s;
            the::text::astring_builder::format_to(s, "%" _I64_PRINTF "u", value);
            this->Serialise(s, name);
        }

        virtual void Serialise(const uint64_t value,
                const wchar_t *name) {
            the::astring s;
            the::text::astring_builder::format_to(s, "%" _I64_PRINTF "u", value);
            this->Serialise(s, name);
        }

        virtual void Serialise(const float value,
                const char *name = NULL) {
            this->Serialise(static_cast<double>(value), name);
        }

        virtual void Serialise(const float value,
                const wchar_t *name) {
            this->Serialise(static_cast<double>(value), name);
        }

        virtual void Serialise(const double value,
                const char *name = NULL) {
            the::astring s;
            the::text::astring_builder::format_to(s, "%.32g", value);
            this->Serialise(s, name);
        }

        virtual void Serialise(const double value,
                const wchar_t *name) {
            the::astring s;
            the::text::astring_builder::format_to(s, "%.32g", value);
            this->Serialise(s, name);
        }

        virtual void Serialise(const the::astring& value,
                const char *name = NULL) {
            if (name != NULL) {
                T tname;
                the::text::string_converter::convert(tname, name);
                for (unsigned int i = 0; i < this->data.Count(); i++) {
                    if (the::text::string_utility::equals(this->data[i].Key(), tname)) {
                        the::text::string_converter::convert(this->data[i].Second(), value);
                        return;
                    }
                }
                this->data.Append(
                    vislib::Pair<T, T>(tname, the::text::string_converter::convert<T>(value)));
            } else {
                this->data.Append(
                    vislib::Pair<T, T>(nullptr, the::text::string_converter::convert<T>(value)));
            }
        }

        virtual void Serialise(const the::astring& value,
                const wchar_t *name) {
            T tname;
            the::text::string_converter::convert(tname, name);
            for (unsigned int i = 0; i < this->data.Count(); i++) {
                if (the::text::string_utility::equals(this->data[i].Key(), tname)) {
                    the::text::string_converter::convert(this->data[i].Second(), value);
                    return;
                }
            }
            this->data.Append(
                vislib::Pair<T, T>(tname, the::text::string_converter::convert<T>(value)));
        }

        virtual void Serialise(const the::wstring& value,
                const char *name = NULL) {
            if (name != NULL) {
                T tname;
                the::text::string_converter::convert(tname, name);
                for (unsigned int i = 0; i < this->data.Count(); i++) {
                    if (the::text::string_utility::equals(this->data[i].Key(), tname)) {
                        the::text::string_converter::convert(this->data[i].Second(), value);
                        return;
                    }
                }
                this->data.Append(
                    vislib::Pair<T, T>(tname, the::text::string_converter::convert<T>(value)));
            } else {
                this->data.Append(
                    vislib::Pair<T, T>(nullptr, the::text::string_converter::convert<T>(value)));
            }
        }

        virtual void Serialise(const the::wstring& value,
                const wchar_t *name) {
            T tname;
            the::text::string_converter::convert(tname, name);
            for (unsigned int i = 0; i < this->data.Count(); i++) {
                if (the::text::string_utility::equals(this->data[i].Key(), tname)) {
                    the::text::string_converter::convert(this->data[i].Second(), value);
                    return;
                }
            }
            this->data.Append(
                vislib::Pair<T, T>(tname, the::text::string_converter::convert<T>(value)));
        }

        /**
         * Parses an input string to the internal representation. This method
         * is usually used for deserialisation.
         *
         * @param str The input string.
         */
        void SetInputString(const typename T::value_type *str);

        /**
         * Parses an input string to the internal representation. This method
         * is usually used for deserialisation.
         *
         * @param str The input string.
         */
        inline void SetInputString(const T &str) {
            this->SetInputString(str);
        }

        /**
         * Sets the position of the next element to be deserialised if driven
         * by order. Usually this method is only used to jump to the first
         * serialised element to restart the deserialisation process.
         *
         * @param i The index of the next element to be deserialised.
         */
        void SetNextDeserialisePosition(unsigned int i = 0) {
            this->nextDePos = 0;
        }

        /**
         * Stores the internal data in a string. This method is usually used
         * after serialisation to receive the resulting string.
         *
         * @param outStr The string receiving the serialised data.
         */
        void GetString(T &outStr) const;

        /**
         * Returns a string of the internal data. This method is usually used
         * after serialisation to receive the resulting string.
         *
         * @return The string holding the serialised data.
         */
        inline T GetString(void) const {
            T tmp;
            this->GetString(tmp);
            return tmp;
        }

    private:

        /**
         * Answers the value for a given name or the next value to be used for
         * deserialisation. As side effect 'nextDePos' is set to the next
         * element.
         *
         * @param name The name of the value to return.
         *
         * @return The found value.
         *
         * @throws Exception in case of an error.
         */
        inline const T& value(const char *name) {
            if (name != NULL) {
                T tname;
                the::text::string_converter::convert(tname, name);
                for (unsigned int i = 0; i < this->data.Count(); i++) {
                    if (the::text::string_utility::equals(this->data[i].Key(), tname)) {
                        this->nextDePos = i + 1;
                        return this->data[i].Value();
                    }
                }
            } else if (this->nextDePos < this->data.Count()) {
                return this->data[this->nextDePos++].Value();
            }
            throw the::no_such_element_exception("deserialisation failed", 
                __FILE__, __LINE__);
        }

        /**
         * Answers the value for a given name or the next value to be used for
         * deserialisation. As side effect 'nextDePos' is set to the next
         * element.
         *
         * @param name The name of the value to return.
         *
         * @return The found value.
         *
         * @throws Exception in case of an error.
         */
        inline const T& value(const wchar_t *name) {
            if (name != NULL) {
                T tname;
                the::text::string_converter::convert(tname, name);
                for (unsigned int i = 0; i < this->data.Count(); i++) {
                    if (the::text::string_utility::equals(this->data[i].Key(), tname)) {
                        this->nextDePos = i + 1;
                        return this->data[i].Value();
                    }
                }
            } else if (this->nextDePos < this->data.Count()) {
                return this->data[this->nextDePos++].Value();
            }
            throw the::no_such_element_exception("deserialisation failed", 
                __FILE__, __LINE__);
        }

        /** The internal data structure */
        vislib::Array<vislib::Pair<T, T> > data;

        /** 
         * The position of the next value to be deserialised, used for order
         * driven deserialisation.
         */
        unsigned int nextDePos;

    };


    /*
     * StringSerialiser<T>::StringSerialiser
     */
    template<class T>
    StringSerialiser<T>::StringSerialiser(const typename T::value_type *str)
            : Serialiser(SERIALISER_SUPPORTS_NAMES), data(), nextDePos(0) {
        this->SetInputString(str);
    }


    /*
     * StringSerialiser<T>::StringSerialiser
     */
    template<class T>
    StringSerialiser<T>::StringSerialiser(const T& str)
            : Serialiser(SERIALISER_SUPPORTS_NAMES), data(), nextDePos(0) {
        this->SetInputString(str.c_str());
    }


    /*
     * StringSerialiser<T>::~StringSerialiser
     */
    template<class T> StringSerialiser<T>::~StringSerialiser(void) {
        // intentionally empty
    }


    /*
     * StringSerialiser<T>::parseInitString
     */
    template<class T>
    void StringSerialiser<T>::SetInputString(const typename T::value_type *str) {
        this->data.Clear();
        this->nextDePos = 0;

        if ((str == NULL) || (*str == static_cast<typename T::value_type>(0))) {
            return;
        }

        T key;
        T value;
        vislib::StringTokeniser<T> tokeniser(str, 
            static_cast<typename T::value_type>('\n'));

        while (tokeniser.HasNext()) {
            const T& line = tokeniser.Next();
            typename T::size_type pos;

            do {
                pos = line.find(static_cast<typename T::value_type>('='));
            } while ((pos != T::npos) && (pos > 0)
                && (line[pos - 1] == static_cast<typename T::value_type>('\\')));

            key = line.substr(0, pos);
            value = line.substr(pos + 1);

            the::text::string_utility::unescape_characters(key,
                static_cast<typename T::value_type>('\\'),
                the::text::string_converter::convert<T>("\n\r="), 
                the::text::string_converter::convert<T>("nr="));
            the::text::string_utility::unescape_characters(value,
                static_cast<typename T::value_type>('\\'),
                the::text::string_converter::convert<T>("\n\r="), 
                the::text::string_converter::convert<T>("nr="));

            this->data.Append(vislib::Pair<T, T>(key, value));
        }
    }


    /*
     * StringSerialiser<T>::GetString
     */
    template<class T>
    void StringSerialiser<T>::GetString(T &outStr) const {
        outStr.clear();
        T str;
        for (unsigned int i = 0; i < this->data.Count(); i++) {
            str = this->data[i].Key();
            the::text::string_utility::escape_characters(str,
                static_cast<typename T::value_type>('\\'),
                the::text::string_converter::convert<T>("\n\r="), 
                the::text::string_converter::convert<T>("nr="));
            outStr.append(str.c_str());
            outStr.append(the::text::string_converter::convert<T>("="));
            str = this->data[i].Value();
            the::text::string_utility::escape_characters(str,
                static_cast<typename T::value_type>('\\'),
                the::text::string_converter::convert<T>("\n\r="), 
                the::text::string_converter::convert<T>("nr="));
            outStr.append(str.c_str());
            outStr.append(the::text::string_converter::convert<T>("\n"));
        }
    }

#undef _I64_PRINTF

    /** Template instantiation for ANSI strings. */
    typedef StringSerialiser<the::astring> StringSerialiserA;

    /** Template instantiation for wide strings. */
    typedef StringSerialiser<the::wstring> StringSerialiserW;

    /** Template instantiation for TCHARs. */
    typedef StringSerialiser<the::tstring> TStringSerialiser;

} /* end namespace vislib */

#if defined(_WIN32) && defined(_MANAGED)
#pragma managed(pop)
#endif /* defined(_WIN32) && defined(_MANAGED) */
#endif /* VISLIB_STRINGSERIALISER_H_INCLUDED */
