#!/bin/sh

theme="$1"
destdir="${DESTDIR}${LIBDIR}/themes/${theme}"
srcdir="${SRCDIR}/lib/themes/${theme}"

echo "Installing theme: ${theme}"

${INSTALLDIR} "${destdir}"

for pixmap in "${srcdir}/"*.xpm
do
    ${INSTALLLIB} "${pixmap}" "${destdir}"
done

for subtheme in "${srcdir}/"*.theme
do
    ${INSTALLLIB} "${subtheme}" "${destdir}"
done
 
if test -f "${srcdir}/"*.pcf
then
    for font in "${srcdir}/"*.pcf
    do
        ${INSTALLLIB} "${font}" "${destdir}"
    done

    if test -x "${MKFONTDIR}"
    then
    (
        cd "${destdir}"; ${MKFONTDIR}
    )
    else
        echo "WARNING: A dummy file has been copied to"
        echo "         ${destdir}/fonts.dir"
        echo "         You better setup your path to point to mkfontdir or use the"
        echo "         --with-mkfontdir option of the configure script."

        ${INSTALLLIB} "${srcdir}/fonts.dir.default" "${destdir}/fonts.dir"
    fi
fi

for xpmdir in ${XPMDIRS}
do
    if test -d "${srcdir}/${xpmdir}"
    then
        ${INSTALLDIR} "${destdir}/${xpmdir}"
        for pixmap in "${srcdir}/${xpmdir}/"*.xpm
        do
            ${INSTALLLIB} "${pixmap}" "${destdir}/${xpmdir}"
        done
    fi
done
