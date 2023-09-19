import re

class my_mult(object):
    def read_mul_results(self, weight_bit, act_bit, approx):
        # file containing all the txt LUTs from questaSim simulation
        file_path = "./../approximate_multiplier/sim/results/" + "results_" + str(weight_bit) + "x" + str(act_bit) + "_approx.txt"  
        res_list = [] 
        a_min = (-1)*2**(act_bit-1)
        a_max = 2**(act_bit-1)  # not included
        w_min = (-1)*2**(weight_bit-1)
        w_max = 2**(weight_bit-1)   # not included
        f = open(file_path, 'r')
        approx_line = "".join(['APPROX','\s*', '=', '\s*', str(approx)])
        while True:
            line = f.readline()
            if not line:
                print ("EOF reached, approx level not found")
                break
            match_approx = re.match(approx_line, line)
            if (match_approx != None):
                for w in range(w_min, w_max):
                    el_list = []
                    my_el = []
                    el_list.append(w)
                    for a in range(a_min, a_max):
                        mul_pattern = "".join(["\s*",str(w), "\s*x\s*", str(a), "\s*=\s*"]) # CHECK order
                        mul_line = f.readline()
                        if not mul_line:
                            print ("EOF reached, no result found")
                            break
                        match_mul = re.match(mul_pattern, mul_line) ## ERRORE
                        if (match_mul != None):
                            rem_string = re.sub(mul_pattern,"", mul_line)
                            # isolate the numeric value
                            res_tmp = int(rem_string)
                            el = [a, res_tmp]
                            my_el.append(el) 
                        else:
                            print ("Pattern",mul_line," not found, error occurred", mul_pattern)
                    el_list.append(my_el)
                    res_list.append(el_list)
                break
                                    
        f.close()
        print("Finished parsing file")
        return res_list
    
    def mul(self, weight, act, res_list):
        list_idx = [el_list[0] for el_list in res_list].index(int(weight))
        sublist_res = res_list[list_idx][1] #at next idx you find all the results
        sublist_idx = [a for a, res_tmp in sublist_res].index(int(act))                                    
        res = sublist_res[sublist_idx][1]
        return res



def generate_mult_LUT(w_w, a_w, approx):
    print("Generate header file for approximate multiplier", w_w, "x", a_w, ",", approx)
    axx_mult = my_mult()
    
    # read the result file into a list
    res_list = axx_mult.read_mul_results(w_w, a_w, approx)
    
    mult_name = 'bw_mult_'+ str(w_w) + '_' + str(a_w) + '_' + str(approx)
    
    #write the header file to the correct path
    with open('./adapt/cpu-kernels/axx_mults/' + mult_name + '.h', 'w') as filehandle: 
        a_bits = int(pow(2,a_w))
        w_bits = int(pow(2,w_w))
        lut_size_w = str(w_bits)
        lut_size_a = str(a_bits)
        filehandle.write('#include <stdint.h>\n\n')
        filehandle.write('const int' + str(32) + '_t lut [' + lut_size_w + '][' + lut_size_a +'] = {')       
        #use 32-bits as the worst case is 18 bit
        for i in range (0,w_bits//2):
            filehandle.write('{')
            for j in range (0,a_bits//2):
                x = axx_mult.mul(i,j,res_list)
                filehandle.write('%s' % x)
                filehandle.write(', ')  
            for j in range (a_bits//2,a_bits):
                x = axx_mult.mul(i,(a_bits-j)*-1, res_list)
                filehandle.write('%s' % x)
                if j!=a_bits-1:
                    filehandle.write(', ') 
            filehandle.write('},')  
            filehandle.write('\n')
             
        for i in range (w_bits//2,w_bits):
            filehandle.write('{')
            for j in range (0,a_bits//2):
                x = axx_mult.mul((w_bits-i)*-1,j, res_list)       
                filehandle.write('%s' % x)
                filehandle.write(', ')  
            for j in range (a_bits//2,a_bits):
                x = axx_mult.mul((w_bits-i)*-1,(a_bits-j)*-1,res_list) 
                filehandle.write('%s' % x)
                if j!=a_bits-1:
                    filehandle.write(', ')
            if(i!=w_bits-1):        
                filehandle.write('},')
                filehandle.write('\n')
        filehandle.write('}};\n')        


