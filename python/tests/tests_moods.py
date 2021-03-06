from collections import defaultdict
from itertools import chain
import sys
from os.path import join, abspath, dirname

LOCAL_DIR = abspath(dirname(__file__))
sys.path.append(abspath(join(LOCAL_DIR, '..')))

from unittest import TestCase

import MOODS


class TestMOODS_Standalone(TestCase):
    """
    This is for MOODS testing without loading any other Promuter libraries.
    """

    def setUp(self):

        # load all the motifs by hand.
        self.motif_matrices = [
             ('zfp4_yrk_3p',
              [[250, 130, 0, 0, 20, 0, 0, 40, 50],
               [100, 50, 0, 0, 0, 0, 10, 0, 0],
               [10, 200, 380, 380, 0, 380, 150, 340, 0],
               [20, 0, 0, 0, 360, 0, 220, 0, 330]]),
             ('ttgR',
              [[7.5, 44.5, 0.0, 0.0, 2.5, 47.5, 8.5],
               [21.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0],
               [3.5000000000000004, 0.0, 0.0, 50.0, 0.0, 0.0, 0.0],
               [18.0, 5.5, 50.0, 0.0, 47.5, 2.5, 43.0]]),
             ('cdaR',
              [[35, 4, 0, 3, 28, 7, 42, 49, 31, 23],
               [1, 7, 0, 57, 20, 46, 5, 1, 6, 5],
               [2, 0, 59, 0, 0, 0, 5, 1, 2, 25],
               [22, 49, 1, 0, 12, 7, 8, 9, 21, 7]]),
             ('p22_cI',
              [[80, 113, 0, 0, 0, 468, 581, 396, 35],
               [68, 20, 581, 0, 0, 35, 0, 10, 0],
               [160, 0, 0, 0, 0, 0, 0, 10, 0],
               [273, 448, 0, 581, 581, 78, 0, 165, 546]]),
             ('rpol_10',
              [[23, 373, 105, 210, 210, 0],
               [43, 0, 66, 51, 97, 19],
               [19, 3, 51, 55, 37, 11],
               [316, 25, 179, 85, 57, 371]]),
             ('zfp7_ZP10165',
              [[50, 0, 0, 50, 0, 0, 0, 0, 50, 0, 50],
               [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
               [0, 0, 0, 0, 0, 50, 50, 50, 0, 50, 0],
               [0, 50, 50, 0, 50, 0, 0, 0, 0, 0, 0]]),
             ('zfp1_efnba2_3p',
              [[3, 6, 0, 0, 6, 112, 0, 0, 6],
               [15, 112, 0, 0, 112, 0, 6, 118, 0],
               [100, 0, 118, 118, 0, 0, 112, 0, 112],
               [0, 0, 0, 0, 0, 6, 0, 0, 0]]),
             ('lacI',
              [[30, 30, 0, 65, 200, 10, 100, 100, 60, 130],
               [30, 100, 0, 165, 30, 230, 30, 30, 30, 30],
               [200, 100, 0, 10, 10, 10, 30, 30, 40, 40],
               [0, 30, 260, 20, 20, 10, 100, 100, 130, 60]]),
             ('tetR',
              [[120, 40, 300, 100, 220, 100, 190, 50],
               [200, 0, 20, 45, 100, 20, 50, 70],
               [20, 300, 50, 20, 20, 200, 100, 150],
               [50, 50, 20, 225, 50, 70, 50, 120]]),
             ('933W_cI',
              [[985, 548, 1544, 0, 0, 1252, 0, 149],
               [0, 801, 0, 0, 0, 20, 1680, 0],
               [11, 33, 184, 1728, 0, 0, 0, 1393],
               [732, 346, 0, 0, 1728, 456, 48, 186]]),
             ('zfp6_ZP10363',
              [[0, 0, 0, 50, 0, 50, 0, 0, 0, 50, 0],
               [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
               [0, 50, 0, 0, 50, 0, 0, 50, 50, 0, 50],
               [50, 0, 50, 0, 0, 0, 50, 0, 0, 0, 0]]),
             ('zfp5_ZN0024',
              [[50, 0, 50, 0, 0, 0, 50, 0, 0, 50, 0],
               [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50],
               [0, 50, 0, 50, 0, 50, 0, 50, 50, 0, 0],
               [0, 0, 0, 0, 50, 0, 0, 0, 0, 0, 0]]),
             ('434_cI',
              [[621, 80, 186, 0, 0, 0, 0],
               [0, 76, 68, 30, 0, 0, 0],
               [0, 375, 0, 0, 0, 659, 0],
               [38, 128, 405, 629, 659, 0, 659]]),
             ('zfp2_dab2_3p',
              [[8, 178, 108, 0, 70, 158, 0, 150, 0],
               [0, 0, 0, 0, 0, 0, 0, 20, 8],
               [170, 0, 50, 178, 108, 20, 178, 0, 170],
               [0, 0, 20, 0, 0, 0, 0, 8, 0]]),
             ('acuR',
              [[0.0, 0.0, 0.0, 0.0, 25.0, 21.0, 17.5, 34.5],
               [0.0, 0.0, 50.0, 2.0, 7.5, 0.0, 0.0, 0.0],
               [50.0, 0.0, 0.0, 0.0, 0.0, 11.5, 0.0, 2.0],
               [0.0, 50.0, 0.0, 48.0, 17.5, 17.5, 32.5, 13.5]]),
             ('rpol_35',
              [[42, 18, 18, 173, 142, 134, 116],
               [0, 0, 6, 135, 140, 50, 90],
               [0, 72, 244, 0, 40, 72, 82],
               [359, 311, 133, 93, 79, 145, 113]]),
             ('zfp8_ZP10457',
              [[0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 50],
               [50, 0, 0, 0, 0, 0, 0, 0, 0, 50, 0],
               [0, 0, 0, 0, 50, 0, 0, 50, 50, 0, 0],
               [0, 50, 50, 50, 0, 50, 50, 0, 0, 0, 0]]),
             ('lambda_cI',
              [[50, 170, 45, 85, 30, 110, 0, 300],
               [310, 50, 0, 0, 0, 0, 0, 0],
               [50, 190, 365, 5, 380, 195, 0, 65],
               [0, 0, 0, 320, 0, 105, 410, 45]]),
             ('rpol_10_ext',
              [[0, 0, 100, 23, 373, 105, 210, 210, 0],
               [60, 0, 100, 43, 0, 66, 51, 97, 19],
               [140, 240, 100, 19, 3, 51, 55, 37, 11],
               [200, 160, 100, 316, 25, 179, 85, 57, 371]])
        ]

        # load all thresholds by hand
        self.thresholds = [
                 1.0699545943859903,
                 0.7897693125951122,
                 0.6726083744651952,
                 1.2680761835227123,
                 1.2225352937342997,
                 0.9676501362039236,
                 0.87457893821108,
                 0.8775565485963774,
                 0.7705603886602805,
                 1.4143797568075485,
                 0.9676501362039236,
                 0.9676501362039236,
                 1.2748406244294834,
                 1.024286840459233,
                 0.6836163375410198,
                 1.4530032582649177,
                 0.9676501362039236,
                 1.1239104755269196,
                 2.6755385519992174
        ]

        # generate the moodssearch object.
        self.moodssearch_obj_fwd = MOODS.MOODSSearch(
            [x[1] for x in self.motif_matrices],
            self.thresholds,
            MOODS.flatbg(),
            7,
            True,
            False)

        self.moodssearch_obj_both = MOODS.MOODSSearch(
            [x[1] for x in self.motif_matrices],
            self.thresholds,
            MOODS.flatbg(),
            7,
            True,
            True)

        #apFAB46 sequence
        self.apFAB46_seq = ('AAAAAGACAATGAAAAGCTTAGTCATGGCGCGCCAAAAAGAGTATTG'
                'ACTTCGCATCTTTTTGTACCTATAATAGATTCATTGCTA')

        #These are the proper hits.
        self.apFAB46_hits = {
         '434_cI': [(57, 4.688399966083337), (-6, 3.3910215941638624)],
         '933W_cI': [(-61, 4.046026313566105)],
         'acuR': [(54, 2.0456700959741143),
                  (-69, 3.325251483672279),
                  (-34, 2.045670095974115),
                  (-0.0, 7.348975004033191)],
         'cdaR': [(29, 3.0419647953759767), (-16, 0.7214691456808224)],
         'lacI': [(-57, 1.8194505679247397), (-40, 1.288724504297107)],
         'lambda_cI': [(39, 1.9439365463638196)],
         'p22_cI': [],
         'rpol_10': [(5, 2.0391667799057487),
                     (67, 5.8335347738270835),
                     (72, 3.2805992559471626),
                     (-68, 4.930825591137652),
                     (-54, 1.3544349004462848),
                     (-40, 2.4522945494642174),
                     (-38, 3.1644672601697525),
                     (-15, 3.193650492100148),
                     (-9, 1.7285308122946124)],
         'rpol_10_ext': [(-40, 3.263862791624584)],
         'rpol_35': [(9, 2.494746699845068),
                     (44, 4.673608724636109),
                     (57, 2.1790487168281887),
                     (58, 2.226020656779173),
                     (60, 2.779856657750668),
                     (61, 1.54573352717996),
                     (75, 1.6937781338634328),
                     (-79, 1.524973502802859),
                     (-74, 2.3203399149336397),
                     (-32, 1.8592143421105267),
                     (-19, 2.55933983358448),
                     (-9, 3.369439254870037),
                     (-8, 3.834192305019384),
                     (-3, 4.054157145090022),
                     (-2, 1.7887495239839581)],
         'tetR': [(0, 2.35082455257586),
                  (4, 2.301877909267698),
                  (9, 0.853460927069236),
                  (10, 2.155196197307788),
                  (33, 1.1931970179468807),
                  (34, 3.1110648177773217),
                  (66, 1.533984872075769),
                  (69, 2.8253186216029587),
                  (73, 1.9382219304721406),
                  (-75, 1.129366669586711),
                  (-64, 2.4065472341224545),
                  (-55, 1.1931970179468807),
                  (-54, 2.8883371533478757),
                  (-17, 1.282891279146519)],
         'ttgR': [(8, 3.09623693618527),
                  (59, 1.1998476766957555),
                  (-75, 4.694180469193636),
                  (-71, 2.2452537163828197)],
         'zfp1_efnba2_3p': [],
         'zfp2_dab2_3p': [],
         'zfp4_yrk_3p': [],
         'zfp5_ZN0024': [],
         'zfp6_ZP10363': [],
         'zfp7_ZP10165': [],
         'zfp8_ZP10457': []}

        # hits computed by the moods search object
        self.raw_hits = self.moodssearch_obj_both.search(self.apFAB46_seq)


    def test_hit_length(self):
        # print(len(self.raw_hits), len(self.motif_matrices))
        assert len(self.raw_hits) == len(self.motif_matrices)

    def test_hit_thresholds(self):
        # print(self.raw_hits[19])
        for threshold, hits in zip(self.thresholds, self.raw_hits):
            for hit_pos, hit_score in hits:
                assert(hit_score >= threshold)

