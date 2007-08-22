# These just tell gnetman to use the cmos symbols, which are gnetman compatible
# Put your own gnetman initialization commands in ${HOME}/.gEDA/gnetmanrc.tcl
component_library "$config_dir/sym/gnetman"
source_library "$config_dir/sch/gnetman"

# These just tell gnetman to use the cmos symbols, which are gnetman compatible
# Put your own gnetman initialization commands in ${HOME}/.gEDA/gnetmanrc.tcl

# These are device definitions for the SPICE reader/writer.  The default is LTSpice.
# Since new devices over-write old ones, simply add add_spice_device commands to
# your local gnetmanrc.tcl file to modify these definitions
set_spice_target ltspice
add_spice_device {'arbitrary behavioral source' B n1 n2 [V=<equation>] [I=<equation>]}
add_spice_device {'capacitor' C n1 n2 <value> [ic=<value>] [Rser=<value>] [Lser=<value>] [Rpar=<value>] [Cpar=<value>] [m=<value>]}
add_spice_device {'diode' D A K <model> [<area>]}
add_spice_device {'voltage dependent voltage' E n1 n2 nc+ nc- <gain>}
add_spice_device {'current dependent current' F n1 n2 <Vnam> <gain>}
add_spice_device {'voltage dependent current' G n1 n2 nc+ nc- <transconductance>}
add_spice_device {'current dependent voltage' H n1 n2 <Vnam> <transres>}
add_spice_device {'independent current source' I n1 n2 <current>}
add_spice_device {'JFET transistor' J D G S <model> [<area>] [off] [IC=<Vds,Vgs>] [temp=<T>]}
add_spice_device {'mutual inductance' K L1 L2 <coefficient>}
add_spice_device {'inductance' L n1 n2 <inductance> [ic=<value>] [Rser=<value>] [Rpar=<value>] [Cpar=<value>] [m=<value>]}
add_spice_device {'MOSFET transistor' M D G S B <model> [L=<length>] [W=<width>] [AD=<area>] [AS=<area>] [PD=<perim>] [PS=<perim>] [NRD=<value>] [NRS=<value>] [off] [IC=<Vds,Vgs,Vbs>] [temp=<T>]}
add_spice_device {'lossy transmission line' O L+ L- R+ R- <model>}
add_spice_device {'bipolar transistor' Q C B E [S] <model> [<area>] [off] [IC=<Vbe,Vce>] [temp=<T>]}
add_spice_device {'resistance' R n1 n2 <value>}
add_spice_device {'voltage controlled switch' S n1 n2 nc+ nc- <model> [on] [off]}
add_spice_device {'lossless transmission line' T L+ L- R+ R- ZO=<value> TD=<value>}
add_spice_device {'uniform RC-line' U n1 n2 ncommon <model> L=<len> [N=<lumps>]}
add_spice_device {'independent voltage source' V n1 n2 <voltage>}
add_spice_device {'current controlled switch' W n1 n2 <Vnam> <model> [on] [off]}
add_spice_device {'MESFET transistor' Z D G S <model> [<area>] [off] [IC=<Vds,Vgs>]}

# Obviously, this model is very preliminary.  A proper model should be built.
set_spice_target hspice
add_spice_device {'resistor' R n1 n2 [<model>] <value>}
add_spice_device {'capacitor' C n1 n2 [<model>] <value>}
add_spice_device {'MOSFET transistor' M D G S B <model> [L=<length>] [W=<width>] [AD=<area>] [AS=<area>]}
add_spice_device {'bipolar transistor' Q C B E <model> [<area>] [off] [IC=<Vbe,Vce>] [temp=<T>]}
add_spice_device {'diode' D A K <model> [AREA=<area>]}

# This is basically based on the hspice model, and is even more preliminary
set_spice_target cdl
add_spice_device {'resistor' R n1 n2 [<model>] <value>}
add_spice_device {'capacitor' C n1 n2 [<model>] <value>}
add_spice_device {'MOSFET transistor' M D G S B <model> [L=<length>] [W=<width>] [AD=<area>] [AS=<area>] [AG=<area>] [CG=<cap>]}
add_spice_device {'bipolar transistor' Q C B E <model> [<area>] [off] [IC=<Vbe,Vce>] [temp=<T>]}
add_spice_device {'diode' D A K <model> [AREA=<area>]}

# These models need to be built
# set_spice_target pspice
# set_spice_target tclspice

# Select LTSpice as the default.  TLSpice rocks!  It's got good docs, too.
set_spice_target ltspice

