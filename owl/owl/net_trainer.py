import math
import sys
import time
import numpy as np
import owl
import owl.net as net
from net_helper import CaffeNetBuilder

if __name__ == "__main__":
    owl.initialize(sys.argv)
    gpu = owl.create_gpu_device(1)
    owl.set_device(gpu)
    
    #prepare the net and solver
    builder = CaffeNetBuilder(sys.argv[1], sys.argv[2])
    owl_net = net.Net()
    builder.build_net(owl_net)
    #builder.init_net_from_file(owl_net, '/home/tianjun/releaseversion/minerva/owl/apps/imagenet_googlenet/Googmodel/epoch0/')
    #builder.init_net_from_file(owl_net, '/home/tianjun/releaseversion/minerva/owl/apps/imagenet_googlenet/VGGmodel/epoch0/')
    builder.init_net_from_file(owl_net, '/home/tianjun/releaseversion/minerva/owl/apps/imagenet_googlenet/Alexmodel/epoch0/')
    
    #set the accuracy layer
    #acc_name = 'loss/top-1'
    #acc_name = 'loss3/top-1'
    acc_name = 'accuracy'
    last = time.time()

    for iteridx in range(owl_net.solver.max_iter):
        owl_net.forward('TRAIN')
        #owl_net.backward('TRAIN')
        #owl_net.weight_update()
        
        accunit = owl_net.units[builder.top_name_to_layer[acc_name][0]]
        print "time: %s" % (time.time() - last)
        print owl_net.batch_size
        print accunit.acc
        last = time.time()

        #decide whether to test
        if (iteridx + 1) % owl_net.solver.test_interval == 0:
            acc_num = 0
            test_num = 0
            for testiteridx in range(owl_net.solver.max_iter):
                owl_net.forward('TEST')
                accunit = owl_net.units[builder.top_name_to_layer[acc_name][0]]
                print "Accuracy this mb: %f" % (accunit.acc)
                acc_num += accunit.acc * accunit.minibatch_size
                test_num += accunit.minibatch_size