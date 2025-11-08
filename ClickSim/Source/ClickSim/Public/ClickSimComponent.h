#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ClickSimComponent.generated.h"


class IInputProcessor;


UCLASS(ClassGroup = (Input), meta = (BlueprintSpawnableComponent))
class YOURPROJECTNAME_API UClickSimComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UClickSimComponent();

	UFUNCTION(BlueprintCallable, Category = "ClickSim")
	void SimulateLeftClick();

	// If true, only fire when some visible/hovered widget exists
	UPROPERTY(EditAnywhere, Category = "ClickSim")
	bool bRequireHoveredWidget = false;

	// If true, we'll put focus back on the game viewport AFTER any open menu (e.g., combo popup) is closed.
	UPROPERTY(EditAnywhere, Category = "ClickSim")
	bool bReturnFocusToViewportAfterClick = true;

	// If true, we’ll disable UMG focus navigation on BeginPlay (safe for game menus). Turn OFF if you rely on DPAD focus
	UPROPERTY(EditAnywhere, Category = "ClickSim")
	bool bDisableUMGFocusNavOnBeginPlay = true;

	// If true, we keep widgets focusable. Turn OFF only if you know you don’t need widget focus at runtime.
	UPROPERTY(EditAnywhere, Category = "ClickSim")
	bool bForceWidgetsNonFocusableOnBeginPlay = false;

	UPROPERTY(EditAnywhere, Category = "ClickSim")
	bool Enabled = true;

	UFUNCTION(BlueprintCallable, Category = "ClickSim")
	void SetEnabled(bool bNewEnabled);

	UFUNCTION(BlueprintCallable, Category = "ClickSim")
	void SetDisabled(bool bNewDisabled);

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void TryBindInput();

	// Only used when we’re intentionally done with UI focus (outside of a click).
	void EnsureViewportFocus() const;

	// These are now optional and only run at BeginPlay if the flags are set.
	void DisableUMGFocusNavigation() const;
	void ForceWidgetsNonFocusable() const;

	// Slate preprocessor
	void RegisterPreprocessor();
	void UnregisterPreprocessor();

	// After a click, refocus viewport when no menus are visible.
	void PollMenusAndRefocus();

	TSharedPtr<IInputProcessor> Preprocessor;

	FTimerHandle BindRetryTimer;
	FTimerHandle PostClickRefocusTimer;
};