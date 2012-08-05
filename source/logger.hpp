/* ============================================================================
 *
 * Copyright (c) 2009-2012, Freescale Semiconductor, Inc.
 *
 * MIT License
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without
 * limitation the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit persons
 * to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 *  File Name : fmc.h
 *  Author    : Serge Lamikhov-Center
 *
 * ========================================================================= */

#ifndef LOGGER_H
#define LOGGER_H

#include <ostream>

namespace logger {

typedef enum { NONE, ERR, WARN, INFO, DBG1, DBG2, DBG3 } log_level_t;

class logger_
{
  public:
    logger_() : log_level( NONE )
    {}
    
    ~logger_()
    {}

    void
    init( std::ostream& ostr, log_level_t new_log_level )
    {
        stream = &ostr;
        set_log_level( new_log_level );
    }

    static logger_&
    get_logger()
    {
        return log;
    }

    log_level_t
    get_log_level()
    {
        return log_level;
    }

    void
    set_log_level( log_level_t new_log_level )
    {
        log_level = new_log_level;
    }

    template< typename t >
    logger_&
    operator <<( t& data )
    {
        *stream << data;
        return *this;
    }

    logger_&
    operator <<( std::ostream& (*manip)(std::ostream &) ) 
    {
        manip( *stream );
        return *this;
    }

    logger_&
    operator <<( std::ios_base& (*manip)(std::ios_base&) ) 
    {
        manip( *stream );
        return *this;
    }

    static std::string
    prefix( log_level_t log_level )
    {
        switch ( log_level )
        {
        case ERR:
            return "ERR : ";
        case WARN:
            return "WARN: ";
        case INFO:
            return "INFO: ";
        case DBG1:
            return "DBG1: ";
        case DBG2:
            return "DBG2: ";
        case DBG3:
            return "DBG3: ";
        default:
            return "";
        }
    }

  private:
    static        logger_ log;
    log_level_t   log_level;
    std::ostream* stream;
};

#ifdef DEFINE_LOGGER_INSTANCE
    logger_ logger_::log;
#endif // DEFINE_LOGGER_INSTANCE

}; // namespace logger

#define LOG_GET() logger::logger_::get_logger()
#define LOG_INIT( stream, level ) LOG_GET().init( stream, level )
#define LOG_LEVEL( level ) LOG_GET().set_log_level( level )
#define LOG( level )                                              \
    if ( level > LOG_GET().get_log_level() ) ; \
    else LOG_GET() << logger::logger_::prefix( level )

#endif // LOGGER_H