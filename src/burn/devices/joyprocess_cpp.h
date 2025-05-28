#pragma once
#include "joyprocess.h"

#ifdef __cplusplus

// C++-only overload and templates
UINT8 ProcessAnalog(INT16 anaval, INT32 reversed, INT32 flags, UINT8 scalemin, UINT8 scalemax, UINT8 centerval);

// ButtonToggle, an easy-to-use state-scannable togglebutton
struct ButtonToggle {
	INT32 state;
	INT32 last_state;

	ButtonToggle() {
		state = 0;
		last_state = 0;
	}

	INT32 Toggle(UINT8 &input) {
		INT32 toggled = 0;
		if (!last_state && input && !bBurnRunAheadFrame) {
			state ^= 1;
			toggled = 1;
		}
		last_state = input;
		input = state;

		return (toggled);
	}

	void Scan() {
		SCAN_VAR(state);
		SCAN_VAR(last_state);
	}
};

template <int N, typename T = UINT8>
struct HoldCoin {
	T prev[N];
	T counter[N];

	void reset() {
		memset(&prev, 0, sizeof(prev));
		memset(&counter, 0, sizeof(counter));
	}

	void scan() {
		SCAN_VAR(prev);
		SCAN_VAR(counter);
	}

	void check(UINT8 num, T &inp, T bit, UINT8 hold_count) {
		if ((prev[num] & bit) != (inp & bit) && (inp & bit) && !counter[num]) {
			counter[num] = hold_count + 1;
		}
		prev[num] = (inp & bit);
		if (counter[num]) {
			counter[num]--;
			inp |= bit;
		}
		if (!counter[num]) {
			inp &= ~bit;
		}
	}

	void checklow(UINT8 num, T &inp, T bit, UINT8 hold_count) {
		if ((prev[num] & bit) != (inp & bit) && (~inp & bit) && !counter[num]) {
			counter[num] = hold_count + 1;
		}
		prev[num] = (inp & bit);
		if (counter[num]) {
			counter[num]--;
			inp &= ~bit;
		}
		if (!counter[num]) {
			inp |= bit;
		}
	}
};

template <int N, typename T>
struct ClearOpposite {
	T prev[N<<2];

	void reset() {
		memset(&prev, 0, sizeof(prev));
	}

	void scan() {
		SCAN_VAR(prev);
	}

	void checkval(UINT8 n, T &inp, T val_a, T val_b) {
		// When opposites become pressed simultaneously, and a 3rd direction isn't pressed
		// remove the previously stored direction if it exists, cancel both otherwise
		if ((inp & val_a) == val_a)
			inp &= (prev[n] && (inp & val_b) == 0 ? (inp ^ prev[n]) : ~val_a);
		// Store direction anytime it's pressed without its opposite
		else if (inp & val_a)
			prev[n] = inp & val_a;
	}

	void check(UINT8 num, T &inp, T val1, T val2) {
		checkval((num<<1)  , inp, val1, val2);
		checkval((num<<1)+1, inp, val2, val1);
	}
};

#endif // __cplusplus 