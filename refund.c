#include "refund.h"
#include "utils.h"

float calculateRefundAmount(float farePaid)
{
    if (farePaid < 0.0f) return 0.0f;
    return farePaid * 0.75f;
}

int confirmRefundAmount(float farePaid, float refundAmount)
{
    printf("\nCancellation deduction (25%%) : Rs. %.2f\n", farePaid * 0.25f);
    printf("Refundable amount             : Rs. %.2f\n", refundAmount);
    for (int attempt = 1; attempt <= 3; ++attempt) {
        float entered = getFloatInput("Enter the exact refundable amount to confirm (Rs.) : ");
        if (entered < 0.0f) { printf("Invalid refund input exhausted its three input attempts. Cancellation was not performed.\n"); return 0; }
        if (entered >= refundAmount - 0.005f && entered <= refundAmount + 0.005f) {
            printf("Refund amount confirmed.\n");
            return 1;
        }
        printf("Amount does not match Rs. %.2f. Attempt %d of 3.\n", refundAmount, attempt);
    }
    printf("Refund confirmation failed. Ticket has NOT been cancelled.\n");
    return 0;
}
