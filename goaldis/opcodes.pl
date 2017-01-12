#------------------------------------------------------------------------------------------------------------------------------------------
# Perl script to generate opcode tables
#------------------------------------------------------------------------------------------------------------------------------------------

use strict;
use warnings;

#------------------------------------------------------------------------------------------------------------------------------------------
my @opcodes = split('\n', <<EOF

# Opcode tables:
# Flags:
#   D - has a delay slot
#   L - likely delay slot behaviour
#   E - ends control flow

# MIPS opcodes
# --- opcode --------------------------------|flag|--- disasm -----------------
000000 00000 00000 00000 00000 000000        | .. | nop                        
000000 rs rt rd 00000 100000                 | .. | add %rd, %rs, %rt          
001000 rs rt simm                            | .. | addi %rt, %rs, %simm       
001001 rs rt simm                            | .. | addiu %rt, %rs, %simm      
000000 rs rt rd 00000 100001                 | .. | addu %rd, %rs, %rt         
000000 rs rt rd 00000 100100                 | .. | and %rd, %rs, %rt          
001100 rs rt zimm                            | .. | andi %rt, %rs, %zimm       
000100 rs rt branch                          | D. | beq %rs, %rt, %branch      
010100 rs rt branch                          | DL | beql %rs, %rt, %branch     
000001 rs 00001 branch                       | D. | bgez %rs, %branch          
000001 rs 10001 branch                       | D. | bgezal %rs, %branch        
000001 rs 10011 branch                       | DL | bgezall %rs, %branch       
000001 rs 00011 branch                       | DL | bgezl %rs, %branch         
000111 rs 00000 branch                       | D. | bgtz %rs, %branch          
010111 rs 00000 branch                       | DL | bgtzl %rs, %branch         
000110 rs 00000 branch                       | D. | blez %rs, %branch          
010110 rs 00000 branch                       | DL | blezl %rs, %branch         
000001 rs 00000 branch                       | D. | bltz %rs, %branch          
000001 rs 10000 branch                       | D. | bltzal %rs, %branch        
000001 rs 10010 branch                       | DL | bltzall %rs, %branch       
000001 rs 00010 branch                       | DL | bltzl %rs, %branch         
000101 rs rt branch                          | D. | bne %rs, %rt, %branch      
010101 rs rt branch                          | DL | bnel %rs, %rt, %branch     
000000 code 001101                           | .. | break %code                
000000 rs rt rd 00000 101100                 | .. | dadd %rd, %rs, %rt         
011000 rs rt simm                            | .. | daddi %rt, %rs, %simm      
011001 rs rt simm                            | .. | daddiu %rt, %rs, %simm     
000000 rs rt rd 00000 101101                 | .. | daddu %rd, %rs, %rt        
000000 rs rt 00000 00000 011010              | .. | div %rs, %rt               
000000 rs rt 00000 00000 011011              | .. | divu %rs, %rt              
000000 00000 rt rd sa 111000                 | .. | dsll %rd, %rt, %sa         
000000 00000 rt rd sa 111100                 | .. | dsll32 %rd, %rt, %sa       
000000 rs rt rd 00000 010100                 | .. | dsllv %rd, %rt, %rs        
000000 00000 rt rd sa 111011                 | .. | dsra %rd, %rt, %sa         
000000 00000 rt rd sa 111111                 | .. | dsra32 %rd, %rt, %sa       
000000 rs rt rd 00000 010111                 | .. | dsrav %rd, %rt, %rs        
000000 00000 rt rd sa 111010                 | .. | dsrl %rd, %rt, %sa         
000000 00000 rt rd sa 111110                 | .. | dsrl32 %rd, %rt, %sa       
000000 rs rt rd 00000 010110                 | .. | dsrlv %rd, %rt, %rs        
000000 rs rt rd 00000 101110                 | .. | dsub %rd, %rs, %rt         
000000 rs rt rd 00000 101111                 | .. | dsubu %rd, %rs, %rt        
# TODO: J/JAL
000000 rs 00000 rd    00000 001001           | D. | jalr %rd, %rs              
000000 rs 00000 00000 00000 001000           | DE | jr %rs                     
100000 base rt offset                        | .. | lb %rt, %offset(%base)     
100100 base rt offset                        | .. | lbu %rt, %offset(%base)    
110111 base rt offset                        | .. | ld %rt, %offset(%base)     
011010 base rt offset                        | .. | ldl %rt, %offset(%base)    
011011 base rt offset                        | .. | ldr %rt, %offset(%base)    
100001 base rt offset                        | .. | lh %rt, %offset(%base)     
100101 base rt offset                        | .. | lhu %rt, %offset(%base)    
001111 00000 rt simm                         | .. | lui %rt, %simm             
100011 base rt offset                        | .. | lw %rt, %offset(%base)     
100010 base rt offset                        | .. | lwl %rt, %offset(%base)    
100110 base rt offset                        | .. | lwr %rt, %offset(%base)    
100111 base rt offset                        | .. | lwu %rt, %offset(%base)    
000000 00000 00000 rd 00000 010000           | .. | mfhi %rd                   
000000 00000 00000 rd 00000 010010           | .. | mflo %rd                   
000000 rs rt rd 00000 001011                 | .. | movn %rd, %rs, %rt         
000000 rs rt rd 00000 001010                 | .. | movz %rd, %rs, %rt         
000000 rs 00000 00000 00000 010001           | .. | mthi %rs                   
000000 rs 00000 00000 00000 010011           | .. | mtlo %rs                   
000000 rs rt rd 00000 011000                 | .. | mult %rd, %rs, %rt         
000000 rs rt rd 00000 011001                 | .. | multu %rd, %rs, %rt        
000000 rs rt rd 00000 100111                 | .. | nor %rd, %rs, %rt          
000000 rs rt rd 00000 100101                 | .. | or %rd, %rs, %rt           
001101 rs rt simm                            | .. | or %rt, %rs, %simm         
110011 base hint offset                      | .. | pref %hint, %offset(%base) 
101000 base rt offset                        | .. | sb %rt, %offset(%base)     
111111 base rt offset                        | .. | sd %rt, %offset(%base)     
# TODO: SDL/SDR
101001 base rt offset                        | .. | sh %rt, %offset(%base)     
000000 00000 rt rd sa 000000                 | .. | sll %rd, %rt, %sa          
000000 rs rt rd 00000 000100                 | .. | sllv %rd, %rt, %rs         
000000 rs rt rd 00000 101010                 | .. | slt %rd, %rs, %rt          
001010 rs rt simm                            | .. | slti %rt, %rs, %simm       
001011 rs rt simm                            | .. | sltiu %rt, %rs, %simm      
000000 rs rt rd 00000 101011                 | .. | sltu %rd, %rs, %rt         
000000 00000 rt rd sa 000011                 | .. | sra %rd, %rt, %sa          
000000 rs rt rd 00000 000111                 | .. | srav %rd, %rt, %rs         
000000 00000 rt rd sa 000010                 | .. | srl %rd, %rt, %sa          
000000 rs rt rd 00000 000110                 | .. | srlv %rd, %rt, %rs         
000000 rs rt rd 00000 100010                 | .. | sub %rd, %rs, %rt          
000000 rs rt rd 00000 100011                 | .. | subu %rd, %rs, %rt         
101011 base rt offset                        | .. | sw %rt, %offset(%base)     
# TODO: SWL/SWR
000000 00000 00000 00000 0.... 001111        | .. | sync.l                     
000000 00000 00000 00000 1.... 001111        | .. | sync.p                     
000000 code 001100                           | .. | syscall %code              
# TODO: TEQ/TEQI/TGE/TGEI/TGEIU/TGEU/TLT/TLTI/TLTIU/TLTU/TNE/TNEI
000000 rs rt rd 00000 100110                 | .. | xor %rd, %rs, %rt          
001110 rs rt zimm                            | .. | xor %rt, %rs, %zimm        


# EE/MMI opcodes
# --- opcode --------------------------------|flag|--- disasm -----------------
011100 rs rt 00000 00000 011010              | .. | div1 %rs, %rt              
011100 rs rt 00000 00000 011011              | .. | divu1 %rs, %rt             
011110 base rt offset                        | .. | lq %rt, %offset(%base)     
011100 rs rt rd 00000 000000                 | .. | madd %rd, %rs, %rt         
011100 rs rt rd 00000 100000                 | .. | madd1 %rd, %rs, %rt        
011100 rs rt rd 00000 000001                 | .. | maddu %rd, %rs, %rt        
011100 rs rt rd 00000 100001                 | .. | maddu1 %rd, %rs, %rt       
011100 00000 00000 rd 00000 010000           | .. | mfhi1 %rd                  
011100 00000 00000 rd 00000 010010           | .. | mflo1 %rd                  
000000 00000 00000 rd 00000 101000           | .. | mfsa %rd                   
011100 rs 00000 00000 00000 010001           | .. | mthi1 %rs                  
011100 rs 00000 00000 00000 010011           | .. | mtlo1 %rs                  
000000 rs 00000 00000 00000 101001           | .. | mtsa %rs                   
000001 rs 11000 zimm                         | .. | mtsab %rs, %zimm           
000001 rs 11001 zimm                         | .. | mtsah %rs, %zimm           
011100 rs rt rd 00000 011000                 | .. | mult1 %rd, %rs, %rt        
011100 rs rt rd 00000 011001                 | .. | multu1 %rd, %rs, %rt       
011100 00000 rt rd 00101 101000              | .. | pabsh %rd, %rt             
011100 00000 rt rd 00001 101000              | .. | pabsw %rd, %rt             
011100 rs rt rd 01000 001000                 | .. | paddb %rd, %rs, %rt        
011100 rs rt rd 00100 001000                 | .. | paddh %rd, %rs, %rt        
011100 rs rt rd 11000 001000                 | .. | paddsb %rd, %rs, %rt       
011100 rs rt rd 10100 001000                 | .. | paddsh %rd, %rs, %rt       
011100 rs rt rd 10000 001000                 | .. | paddsw %rd, %rs, %rt       
011100 rs rt rd 11000 101000                 | .. | paddub %rd, %rs, %rt       
011100 rs rt rd 10100 101000                 | .. | padduh %rd, %rs, %rt       
011100 rs rt rd 10000 101000                 | .. | padduw %rd, %rs, %rt       
011100 rs rt rd 00000 001000                 | .. | paddw %rd, %rs, %rt        
011100 rs rt rd 00100 101000                 | .. | padsbh %rd, %rs, %rt       
011100 rs rt rd 10010 001001                 | .. | pand %rd, %rs, %rt         
011100 rs rt rd 01010 101000                 | .. | pceqb %rd, %rs, %rt        
011100 rs rt rd 00110 101000                 | .. | pceqh %rd, %rs, %rt        
011100 rs rt rd 00010 101000                 | .. | pceqw %rd, %rs, %rt        
011100 rs rt rd 01010 001000                 | .. | pcgtb %rd, %rs, %rt        
011100 rs rt rd 00110 001000                 | .. | pcgth %rd, %rs, %rt        
011100 rs rt rd 00010 001000                 | .. | pcgtw %rd, %rs, %rt        
011100 00000 rt rd 11011 101001              | .. | pcpyh %rd, %rt             
011100 rs rt rd 01110 001001                 | .. | pcpyld %rd, %rs, %rt       
011100 rs rt rd 01110 101001                 | .. | pcpyud %rd, %rs, %rt       
011100 rs rt 00000 11101 001001              | .. | pdivbw %rs, %rt            
011100 rs rt 00000 01101 101001              | .. | pdivuw %rs, %rt            
011100 rs rt 00000 01101 001001              | .. | pdivw %rs, %rt             
011100 00000 rt rd 11010 101001              | .. | pexch %rd, %rt             
011100 00000 rt rd 11110 101001              | .. | pexcw %rd, %rt             
011100 00000 rt rd 11010 001001              | .. | pexeh %rd, %rt             
011100 00000 rt rd 11110 001001              | .. | pexew %rd, %rt             
011100 00000 rt rd 11110 001000              | .. | pext5 %rd, %rt             
011100 rs rt rd 11010 001000                 | .. | pextlb %rd, %rs, %rt       
011100 rs rt rd 10110 001000                 | .. | pextlh %rd, %rs, %rt       
011100 rs rt rd 10010 001000                 | .. | pextlw %rd, %rs, %rt       
011100 rs rt rd 11010 101000                 | .. | pextub %rd, %rs, %rt       
011100 rs rt rd 10110 101000                 | .. | pextuh %rd, %rs, %rt       
011100 rs rt rd 10010 101000                 | .. | pextuw %rd, %rs, %rt       
011100 rs rt rd 10001 001001                 | .. | phmadh %rd, %rs, %rt       
011100 rs rt rd 10101 001001                 | .. | phmsbh %rd, %rs, %rt       
011100 rs rt rd 01010 101001                 | .. | pinteh %rd, %rs, %rt       
011100 rs rt rd 01010 001001                 | .. | pinth %rd, %rs, %rt        
011100 rs 00000 rd 00000 000100              | .. | plzcw %rd, %rs             
011100 rs rt rd 10000 001001                 | .. | pmaddh %rd, %rs, %rt       
011100 rs rt rd 00000 101001                 | .. | pmadduw %rd, %rs, %rt      
011100 rs rt rd 00000 001001                 | .. | pmaddw %rd, %rs, %rt       
011100 rs rt rd 00111 001000                 | .. | pmaxh %rd, %rs, %rt        
011100 rs rt rd 00011 001000                 | .. | pmaxw %rd, %rs, %rt        
011100 00000 00000 rd 01000 001001           | .. | pmfhi %rd                  
011100 00000 00000 rd 00011 110000           | .. | pmfhl.lh %rd               
011100 00000 00000 rd 00000 110000           | .. | pmfhl.lw %rd               
011100 00000 00000 rd 00100 110000           | .. | pmfhl.sh %rd               
011100 00000 00000 rd 00010 110000           | .. | pmfhl.slw %rd              
011100 00000 00000 rd 00001 110000           | .. | pmfhl.uw %rd               
011100 00000 00000 rd 01001 001001           | .. | pmflo %rd                  
011100 rs rt rd 00111 101000                 | .. | pminh %rd, %rs, %rt        
011100 rs rt rd 00011 101000                 | .. | pminw %rd, %rs, %rt        
011100 rs rt rd 10100 001001                 | .. | pmsubh %rd, %rs, %rt       
011100 rs rt rd 00100 001001                 | .. | pmsubw %rd, %rs, %rt       
011100 rs 00000 00000 01000 101001           | .. | pmthi %rs                  
011100 rs 00000 00000 00000 110001           | .. | pmthl.lw %rs               
011100 rs 00000 00000 01001 101001           | .. | pmtlo %rs                  
011100 rs rt rd 11100 001001                 | .. | pmulth %rd, %rs, %rt       
011100 rs rt rd 01100 101001                 | .. | pmultuw %rd, %rs, %rt      
011100 rs rt rd 01100 001001                 | .. | pmultw %rd, %rs, %rt       
011100 rs rt rd 10011 101001                 | .. | pnor %rd, %rs, %rt         
011100 rs rt rd 10010 101001                 | .. | por %rd, %rs, %rt          
011100 00000 rt rd 11111 001000              | .. | ppac5 %rd, %rt             
011100 rs rt rd 11011 001000                 | .. | ppacb %rd, %rs, %rt        
011100 rs rt rd 10111 001000                 | .. | ppach %rd, %rs, %rt        
011100 rs rt rd 10011 001000                 | .. | ppacw %rd, %rs, %rt        
011100 00000 rt rd 11011 001001              | .. | prevh %rd, %rt             
011100 00000 rt rd 11111 001001              | .. | prot3w %rd, %rt            
011100 00000 rt rd sa 110100                 | .. | psllh %rd, %rt, %sa        
011100 rs rt rd 00010 001001                 | .. | psllvw %rd, %rt, %rs       
011100 00000 rt rd sa 111100                 | .. | psllw %rd, %rt, %sa        
011100 00000 rt rd sa 110111                 | .. | psrah %rd, %rt, %sa        
011100 rs rt rd 00011 101001                 | .. | psravw %rd, %rt, %rs       
011100 00000 rt rd sa 111111                 | .. | psraw %rd, %rt, %sa        
011100 00000 rt rd sa 110110                 | .. | psrlh %rd, %rt, %sa        
011100 rs rt rd 00011 001001                 | .. | psrlvw %rd, %rt, %rs       
011100 00000 rt rd sa 111110                 | .. | psrlw %rd, %rt, %sa        
011100 rs rt rd 01001 001000                 | .. | psubb %rd, %rs, %rt        
011100 rs rt rd 00101 001000                 | .. | psubh %rd, %rs, %rt        
011100 rs rt rd 11001 001000                 | .. | psubsb %rd, %rs, %rt       
011100 rs rt rd 10101 001000                 | .. | psubsh %rd, %rs, %rt       
011100 rs rt rd 10001 001000                 | .. | psubsw %rd, %rs, %rt       
011100 rs rt rd 10001 001000                 | .. | psubub %rd, %rs, %rt       
011100 rs rt rd 10101 101000                 | .. | psubuh %rd, %rs, %rt       
011100 rs rt rd 10001 101000                 | .. | psubuw %rd, %rs, %rt       
011100 rs rt rd 00001 001000                 | .. | psubw %rd, %rs, %rt        
011100 rs rt rd 10011 001001                 | .. | pxor %rd, %rs, %rt         
011100 rs rt rd 11011 101000                 | .. | qfsrv %rd, %rs, %rt        
011111 base rt offset                        | .. | sq %rt, %offset(%base)     


# COP0/MMU opcodes
# --- opcode --------------------------------|flag|--- disasm -----------------
010000 10000 00000 00000 00000 111001        | .. | di                         
010000 10000 00000 00000 00000 111000        | .. | ei                         
101111 base hint offset                      | .. | cache %hint, %offset(%base)
010000 10000 00000 00000 00000 011000        | .. | eret                       
010000 00000 rt cpr 00000 000000             | .. | mfc0 %rt, %cpr             
010000 00000 rt 11001 00000 cpr 1            | .. | mfpc %rt, %cpr             
010000 00100 rt cpr 00000 000000             | .. | mtc0 %rt, %cpr             
010000 00100 rt 11000 00000 000100           | .. | mtdab %rt                  
010000 00100 rt 11000 00000 000101           | .. | mtdabm %rt                 
010000 00100 rt 11001 00000 cpr 1            | .. | mtpc %rt, %cpr             
# TODO: the rest


# COP1/FPU opcodes
# --- opcode --------------------------------|flag|--- disasm -----------------
010001 10000 00000 fs fd 000101              | .. | abs.s %fd, %fs             
010001 10000 ft fs fd 000000                 | .. | add.s %fd, %fs, %ft        
010001 10000 ft fs 00000 011000              | .. | adda.s %fs, %ft            
010001 01000 00000 branch                    | D. | bc1f %branch               
010001 01000 00010 branch                    | DL | bc1fl %branch              
010001 01000 00001 branch                    | D. | bc1t %branch               
010001 01000 00011 branch                    | DL | bc1tl %branch              
010001 10000 ft fs 00000 110010              | .. | c.eq.s %fs, %ft            
010001 10000 ft fs 00000 110000              | .. | c.f.s %fs, %ft             
010001 10000 ft fs 00000 110110              | .. | c.le.s %fs, %ft            
010001 10000 ft fs 00000 110100              | .. | c.lt.s %fs, %ft            
010001 00010 rt fcr 00000 000000             | .. | cfc1 %rt, %fcr             
010001 00110 rt fcr 00000 000000             | .. | ctc1 %rt, %fcr             
010001 10100 00000 fs fd 100000              | .. | cvt.s.w %fd, %fs           
010001 10000 00000 fs fd 100100              | .. | cvt.w.s %fd, %fs           
010001 10000 ft fs fd 000011                 | .. | div.s %fd, %fs, %ft        
110001 base ft offset                        | .. | lwc1 %ft, %offset(%base)   
010001 10000 ft fs fd 011100                 | .. | madd.s %fd, %fs, %ft       
010001 10000 ft fs 00000 011110              | .. | madda.s %fs, %ft           
010001 10000 ft fs fd 101000                 | .. | max.s %fd, %fs, %ft        
010001 00000 rt fs 00000 000000              | .. | mfc1 %rt, %fs              
010001 10000 ft fs fd 101001                 | .. | min.s %fd, %fs, %ft        
010001 10000 00000 fs fd 000110              | .. | mov.s %fd, %fs             
010001 10000 ft fs fd 011101                 | .. | msub.s %fd, %fs, %ft       
010001 10000 ft fs 00000 011111              | .. | msuba.s %fs, %ft           
010001 00100 rt fs 00000 000000              | .. | mtc1 %rt, %fs              
010001 10000 ft fs fd 000010                 | .. | mul.s %fd, %fs, %ft        
010001 10000 ft fs 00000 011010              | .. | mula.s %fs, %ft            
010001 10000 00000 fs fd 000111              | .. | neg.s %fd, %fs             
010001 10000 ft fs fd 010110                 | .. | rsqrt.s %fd, %fs, %ft      
010001 10000 ft 00000 fd 000100              | .. | sqrt.s %fd, %ft            
010001 10000 ft fs fd 000001                 | .. | sub.s %fd, %fs, %ft        
010001 10000 ft fs 00000 011001              | .. | suba.s %fs, %ft            
111001 base ft offset                        | .. | swc1 %ft, %offset(%base)   


# COP2/VU opcodes
# --- opcode --------------------------------|flag|--- disasm --------------------
010010 01000 00000 branch                    | D. | bc2f %branch                  
010010 01000 00010 branch                    | DL | bc2fl %branch                 
010010 01000 00001 branch                    | D. | bc2t %branch                  
010010 01000 00011 branch                    | DL | bc2tl %branch                 
010010 00010 rt vid 00000 000000             | .. | cfc2 %rt, %vid                
010010 00010 rt vid 00000 000001             | .. | cfc2.i %rt, %vid              
010010 00110 rt vid 00000 000000             | .. | ctc2 %rt, %vid                
010010 00110 rt vid 00000 000001             | .. | ctc2.i %rt, %vid              
110110 base vft offset                       | .. | lqc2 %vft, %offset(%base)     
010010 00001 rt vfd 00000 000000             | .. | qmfc2 %rt, %vfd               
010010 00001 rt vfd 00000 000001             | .. | qmfc2.i %rt, %vfd             
010010 00101 rt vfd 00000 000000             | .. | qmtc2 %rt, %vfd               
010010 00101 rt vfd 00000 000001             | .. | qmtc2.i %rt, %vfd             
111110 base vft offset                       | .. | sqc2 %vft, %offset(%base)     
010010 1 wm vft vfs 00111 111101             | .. | vabs.%wm %vft, %vfs           
010010 1 wm vft vfs vfd 101000               | .. | vadd.%wm %vfd, %vfs, %vft     
010010 1 wm 00000 vfs vfd 100010             | .. | vaddi.%wm %vfd, %vfs, I       
010010 1 wm 00000 vfs vfd 100000             | .. | vaddq.%wm %vfd, %vfs, Q       
010010 1 wm vft vfs vfd 0000 bc              | .. | vadd%bc.%wm %vfd, %vfs, %vft  
010010 1 wm vft vfs 01010 111100             | .. | vadda.%wm acc, %vfs, %vft     
010010 1 wm 00000 vfs 01000 111110           | .. | vaddai.%wm acc, %vfs, I       
010010 1 wm 00000 vfs 01000 111100           | .. | vaddaq.%wm acc, %vfs, Q       
010010 1 wm vft vfs 00000 1111 bc            | .. | vadda%bc.%wm acc, %vfs, %vft  
010010 1 0000 vcall 111000                   | .. | vcallms %vcall                
010010 1 0000 00000 vis 00000 111001         | .. | vcallmsr %vis                 
010010 1 wm vft vfs 00111 111111             | .. | vclipw.xyz %vfs, %vft         
010010 1 ftf fsf vft vfs 01110 111100        | .. | vdiv Q, %vfs.%fsf, %vft.%ftf  
010010 1 wm vft vfs 00101 111100             | .. | vftoi0.%wm %vft, %vfs         
010010 1 wm vft vfs 00101 111101             | .. | vftoi4.%wm %vft, %vfs         
010010 1 wm vft vfs 00101 111110             | .. | vftoi12.%wm %vft, %vfs        
010010 1 wm vft vfs 00101 111111             | .. | vftoi15.%wm %vft, %vfs        
010010 1 0000 vit vis vid 110000             | .. | viadd %vid, %vis, %vit        
010010 1 0000 vit vis vimm 110010            | .. | viaddi %vit, %vis, %vimm      
010010 1 0000 vit vis vid 110100             | .. | viand %vid, %vis, %vit        
010010 1 wm vit vis 01111 111110             | .. | vilwr.%wm %vit, (%vis)        
010010 1 0000 vit vis vid 110101             | .. | vior %vid, %vis, %vit         
010010 1 0000 vit vis vid 110001             | .. | visub %vid, %vis, %vit        
010010 1 wm vit vis 01111 111111             | .. | viswr.%wm %vit, (%vis)        
010010 1 wm vft vfs 00100 111100             | .. | vitof0.%wm %vft, %vfs         
010010 1 wm vft vfs 00100 111101             | .. | vitof4.%wm %vft, %vfs         
010010 1 wm vft vfs 00100 111110             | .. | vitof12.%wm %vft, %vfs        
010010 1 wm vft vfs 00100 111111             | .. | vitof15.%wm %vft, %vfs        
010010 1 wm vft vis 01101 111110             | .. | vlqd.%wm %vft, (--%vis)       
010010 1 wm vft vis 01101 111100             | .. | vlqi.%wm %vft, (%vis++)       
010010 1 wm vft vfs vfd 101001               | .. | vmadd.%wm %vfd, %vfs, %vft    
010010 1 wm 00000 vfs vfd 100011             | .. | vmaddi.%wm %vfd, %vfs, I      
010010 1 wm 00000 vfs vfd 100001             | .. | vmaddq.%wm %vfd, %vfs, Q      
010010 1 wm vft vfs vfd 0010 bc              | .. | vmadd%bc.%wm %vfd, %vfs, %vft 
010010 1 wm vft vfs 01010 111101             | .. | vmadda.%wm acc, %vfs, %vft    
010010 1 wm 00000 vfs 01000 111111           | .. | vmaddai.%wm acc, %vfs, I      
010010 1 wm 00000 vfs 01000 111101           | .. | vmaddaq.%wm acc, %vfs, Q      
010010 1 wm vft vfs 00010 1111 bc            | .. | vmadda%bc.%wm acc, %vfs, %vft 
010010 1 wm vft vfs vfd 101011               | .. | vmax.%wm %vfd, %vfs, %vft     
010010 1 wm 00000 vfs vfd 011101             | .. | vmaxi.%wm %vfd, %vfs, I       
010010 1 wm vft vfs vfd 0100 bc              | .. | vmax%bc.%wm %vfd, %vfs, %vft  
010010 1 wm vft vis 01111 111101             | .. | vmfir.%wm %vft, %vis          
010010 1 wm vft vfs vfd 101111               | .. | vmin.%wm %vfd, %vfs, %vft     
010010 1 wm 00000 vfs vfd 011111             | .. | vmini.%wm %vfd, %vfs, I       
010010 1 wm vft vfs vfd 0101 bc              | .. | vmin%bc.%wm %vfd, %vfs, %vft  
010010 1 wm vft vfs 01100 111100             | .. | vmove.%wm %vft, %vfs          
010010 1 wm vft vfs 01100 111101             | .. | vmr32.%wm %vft, %vfs          
010010 1 wm vft vfs vfd 101101               | .. | vmsub.%wm %vfd, %vfs, %vft    
010010 1 wm 00000 vfs vfd 100111             | .. | vmsubi.%wm %vfd, %vfs, I      
010010 1 wm 00000 vfs vfd 100101             | .. | vmsubq.%wm %vfd, %vfs, Q      
010010 1 wm vft vfs vfd 0011 bc              | .. | vmsub%bc.%wm %vfd, %vfs, %vft 
010010 1 wm vft vfs 01011 111101             | .. | vmsuba.%wm acc, %vfs, %vft    
010010 1 wm 00000 vfs 01001 111111           | .. | vmsubai.%wm acc, %vfs, I      
010010 1 wm 00000 vfs 01001 111101           | .. | vmsubaq.%wm acc, %vfs, Q      
010010 1 wm vft vfs 00011 1111 bc            | .. | vmsuba%bc.%wm acc, %vfs, %vft 
010010 1 00 fsf vit vfs 01111 111100         | .. | vmtir %vit, %vfs.%fsf         
010010 1 wm vft vfs vfd 101010               | .. | vmul.%wm %vfd, %vfs, %vft    
010010 1 wm 00000 vfs vfd 011110             | .. | vmuli.%wm %vfd, %vfs, I      
010010 1 wm 00000 vfs vfd 011100             | .. | vmulq.%wm %vfd, %vfs, Q      
010010 1 wm vft vfs vfd 0110 bc              | .. | vmul%bc.%wm %vfd, %vfs, %vft 
010010 1 wm vft vfs 01010 111110             | .. | vmula.%wm acc, %vfs, %vft    
010010 1 wm 00000 vfs 00111 111110           | .. | vmulai.%wm acc, %vfs, I      
010010 1 wm 00000 vfs 00111 111100           | .. | vmulaq.%wm acc, %vfs, Q      
010010 1 wm vft vfs 00110 1111 bc            | .. | vmula%bc.%wm acc, %vfs, %vft 
010010 1 0000 00000 00000 01011 111111       | .. | vnop                         
010010 1 1110 vft vfs 01011 111110           | .. | vopmula.xyz acc, %vfs, %vft  
010010 1 1110 vft vfs vfd 101110             | .. | vopmsub.xyz %vfd, %vfs, %vft 
010010 1 wm vft 00000 10000 111101           | .. | vrget.%wm %vft, R             
010010 1 00 fsf 00000 vfs 10000 111110       | .. | vrinit R, %vfs.%fsf           
010010 1 wm vft 00000 10000 111100           | .. | vrnext.%wm %vft, R            
010010 1 ftf fsf vft vfs 01110 111110        | .. | vrsqrt Q, %vfs.%fsf, %vft.%ftf 
010010 1 00 fsf 00000 vfs 10000 111111       | .. | vrxor R, %vfs.%fsf            
010010 1 wm vit vfs 01101 111111             | .. | vsqd.%wm %vfs, (--%vit)       
010010 1 wm vit vfs 01101 111101             | .. | vsqi.%wm %vfs, (%vit++)       
010010 1 ftf 00 vft 00000 01110 111101       | .. | vsqrt Q, %vft.%ftf            
010010 1 wm vft vfs vfd 101100               | .. | vsub.%wm %vfd, %vfs, %vft     
010010 1 wm 00000 vfs vfd 100110             | .. | vsubi.%wm %vfd, %vfs, I       
010010 1 wm 00000 vfs vfd 100100             | .. | vsubq.%wm %vfd, %vfs, Q       
010010 1 wm vft vfs vfd 0001 bc              | .. | vsub%bc.%wm %vfd, %vfs, %vft  
010010 1 wm vft vfs 01011 111100             | .. | vsuba.%wm acc, %vfs, %vft     
010010 1 wm 00000 vfs 01001 111110           | .. | vsubai.%wm acc, %vfs, I       
010010 1 wm 00000 vfs 01001 111100           | .. | vsubaq.%wm acc, %vfs, Q       
010010 1 wm vft vfs 00001 1111 bc            | .. | vsuba%bc.%wm acc, %vfs, %vft  
010010 1 0000 00000 00000 01110 111111       | .. | vwaitq                        


# Fallbacks
# --- opcode --------------------------------|flag|--- disasm --------------------------------
000000 rs rt rd sa of                        | .. | ? SPECIAL opcode %rs, %rt, %rd, %sa, %of  
000001 rs rt rd sa of                        | .. | ? REGIMM opcode %rs, %rt, %rd, %sa, %of   
010000 rs rt rd sa of                        | .. | ? COP0 opcode %rs, %rt, %rd, %sa, %of     
010001 rs rt rd sa of                        | .. | ? COP1 opcode %rs, %rt, %rd, %sa, %of     
010010 rs rt rd sa of                        | .. | ? COP2 opcode %rs, %rt, %rd, %sa, %of     
011100 rs rt rd sa of                        | .. | ? MMI opcode %rs, %rt, %rd, %sa, %of      
op     rs rt rd sa of                        | .. | ? MIPS opcode %op %rs, %rt, %rd, %sa, %of 

EOF
);


#------------------------------------------------------------------------------------------------------------------------------------------

my %builtins = (
	op=>6, rs=>5, rt=>5, rd=>5, sa=>5, of=>6, 
	fs=>5, ft=>5, fd=>5, fcr=>5,
	vis=>5, vit=>5, vid=>5,
	vfs=>5, vft=>5, vfd=>5,
	base=>5, offset=>16, branch=>16, 
	zimm=>16, simm=>16, code=>20, hint=>5,
	wm=>4, vcall=>15, vimm=>5, bc=>2, ftf=>2, fsf=>2,
	cpr=>5
);

sub trim { my $s = shift; $s =~ s/^\s+|\s+$//g; return $s };

open OUT, ">opcodes.h" or die;

print OUT "// Generated by opcodes.pl\n";
print OUT "\tuint32_t " . join(', ', keys %builtins) . ";\n\n";
foreach (@opcodes)
{
	chomp;
	
	if (s/#(.*)$//) {
		# strip comments
		print OUT "\t//$1\n";
	}
	next unless $_;
	
	my $line = $_;
	my @parts = split('\|', $line);
	my @fields = split(' ', $parts[0]);
	
	my $total = 0;
	my $mask = "";
	my $value = "";
	my @vars;
	foreach (@fields)
	{
		if (/^[01.]+$/) {
			for my $c (split //) {
				if ($c eq '.') {
					$mask .= "0";
					$value .= "0";
				} else {
					$mask .= "1";
					$value .= $c;
				}
				$total += 1;
			}
		} else {
			my $len = $builtins{$_} or die "Unknown builtin '$_'\n$line'\n";
			$mask .= "0" x $len;
			$value .= "0" x $len;
			$total += $len;
			push @vars, { name=>$_, offset=>(32-$total), len=>$len };
		}
	}
	
	die "$line\nTotal does not equal 32-bits (=$total).\n" unless $total == 32;
	
	# bin to hex
	$mask  = unpack ("H*", pack ("B*", $mask));
	$value = unpack ("H*", pack ("B*", $value));
	
	my $flags = trim($parts[1]);
	$flags =~ s/\.//g;
	
	# Pull out variables from the ASM.
	my $asm = trim($parts[2]);
	my @asmvarlist;
	$asm =~ s/%(\w+)/ push @asmvarlist, $1; "%s" /eg;
	my $asmargs = join('', map { ", str_$_($_).c_str()" } @asmvarlist);
	
	
	# Generate a decoder.
	print OUT "\tif ((opcode & 0x$mask) == 0x$value) { ";
	foreach my $v (@vars) {
		my $n = $v->{name};
		my $m = (1 << $v->{len}) - 1;
		my $sh = $v->{offset};
		print OUT "$n = (opcode >> $sh) & $m; ";
	}
	for my $c (split //, $flags) {
		print OUT "handle_$c(); ";
	}
	if ($asm =~ /^\? (.+)/) {
		print OUT "die(\"Unknown $1\\n\"$asmargs); ";
	} else {
		print OUT "WRITE(\"\\t$asm\\n\"$asmargs); ";
	}
	print OUT "return; }\n";
}
print OUT "\tdie(\"Missing decode handler for 0x%08x\\n\", opcode);\n";
close OUT;
