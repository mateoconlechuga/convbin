SECTIONS
{
    .bootstrap 0 : { *(.bootstrap) }
    .relocated ti.plotSScreen : AT(SIZEOF(.bootstrap)) { *(.relocated) }
}
